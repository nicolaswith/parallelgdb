#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.hpp"
#include "server.hpp"

const int max_length = 0x2000; // socat default
const asio::ip::port_type base_port_gdb = 0x8000;
const asio::ip::port_type base_port_trgt = 0xC000;

Gtk::Application *g_app;

int run_cmd(ssh_session &session, UIDialog &dialog)
{
	ssh_channel channel;
	int rc;

	channel = ssh_channel_new(session);
	if (channel == NULL)
	{
		return SSH_ERROR;
	}

	rc = ssh_channel_open_session(channel);
	if (rc != SSH_OK)
	{
		ssh_channel_free(channel);
		return rc;
	}

	string cmd = string("srun");
	cmd += string(" --nodes=") + std::to_string(dialog.num_nodes());
	cmd += string(" --ntasks=") + std::to_string(dialog.num_tasks());
	cmd += string(" -p ") + dialog.partition();
	cmd += string(" --mpi=pmi2");
	cmd += string(" ~/client");
	cmd += string(" -s ") + dialog.ip_address();
	cmd += string(" ") + dialog.target();

	rc = ssh_channel_request_exec(channel, cmd.c_str());
	if (rc != SSH_OK)
	{
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		return rc;
	}

	ssh_channel_send_eof(channel);
	ssh_channel_close(channel);
	ssh_channel_free(channel);

	return SSH_OK;
}

bool start_clients_srun(UIDialog &dialog)
{
	ssh_session session;
	int rc;

	session = ssh_new();
	if (session == NULL)
	{
		return false;
	}

	ssh_options_set(session, SSH_OPTIONS_HOST, dialog.ssh_address());
	ssh_options_set(session, SSH_OPTIONS_USER, dialog.ssh_user());

	rc = ssh_connect(session);
	if (rc != SSH_OK)
	{
		fprintf(stderr, "Error connecting to %s: %s\n", dialog.ssh_address(), ssh_get_error(session));
		return false;
	}

	if (SSH_KNOWN_HOSTS_OK != ssh_session_is_known_server(session))
	{
		fprintf(stderr, "The server is not valid.\n");
		ssh_disconnect(session);
		ssh_free(session);
		return false;
	}

	rc = ssh_userauth_password(session, NULL, dialog.ssh_password());
	if (rc != SSH_AUTH_SUCCESS)
	{
		fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(session));
		ssh_disconnect(session);
		ssh_free(session);
		return false;
	}

	rc = run_cmd(session, dialog);
	if (rc != SSH_OK)
	{
		fprintf(stderr, "Error starting clients: %s\n", ssh_get_error(session));
		ssh_disconnect(session);
		ssh_free(session);
		return false;
	}

	ssh_disconnect(session);
	ssh_free(session);

	return true;
}

int start_clients_mpi(const int a_num_processes, const char *const a_target)
{
	const int pid = fork();
	if (0 == pid)
	{
		const char *np_str = strdup(std::to_string(a_num_processes).c_str());

		char *argv[] = {
			// (char *)"/usr/bin/xterm",
			// (char *)"-hold",
			// (char *)"-e",
			(char *)"/usr/bin/mpirun",
			(char *)"-np",
			(char *)np_str,
			(char *)"/home/nicolas/ma/parallelgdb/bin/client",
			(char *)"-m",
			(char *)a_target,
			(char *)nullptr};
		execvp(argv[0], argv);
		_exit(127);
	}
	return pid;
}

void process_session(tcp::socket a_sock, UIWindow &a_window, const int a_port)
{
	const int process_rank = get_process_rank(a_port);
	mi_h *h = nullptr;
	if (src_is_gdb(a_port))
	{
		a_window.m_conns_gdb[process_rank] = &a_sock;
		a_window.m_conns_open_gdb[process_rank] = true;
		h = mi_alloc_h();
		h->line = (char *)malloc(max_length * sizeof(char));
	}
	else
	{
		a_window.m_conns_trgt[process_rank] = &a_sock;
	}

	try
	{
		for (;;)
		{
			char *data = (char *)malloc((max_length + 8) * sizeof(char));
			if (nullptr == data)
			{
				throw std::bad_alloc();
			}

			asio::error_code error;
			const size_t length = a_sock.read_some(asio::buffer(data, max_length), error);
			if (asio::error::eof == error)
			{
				if (src_is_gdb(a_port))
				{
					a_window.m_conns_open_gdb[process_rank] = false;
				}
				break;
			}
			else if (error)
			{
				throw asio::system_error(error);
			}

			// idx 0 to (length-1) is valid data.
			// add null termination to received data.
			data[length] = '\0';

			Glib::signal_idle().connect_once(
				sigc::bind(
					sigc::mem_fun(
						a_window,
						&UIWindow::print_data),
					h,
					strdup(data),
					length + 1, // now length+1 chars are valid (inc null termination)
					a_port));

			free(data);
		}
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception in thread: " << e.what() << "\n";
	}
}

void start_acceptor(UIWindow &a_window, const asio::ip::port_type a_process_port)
{
	asio::io_context io_context;
	tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), a_process_port));
	process_session(acceptor.accept(), a_window, a_process_port);
}

void start_servers(UIWindow &a_window)
{
	for (int i = 0; i < a_window.m_num_processes; ++i)
	{
		std::thread(start_acceptor, std::ref(a_window), (base_port_gdb + i)).detach();
		std::thread(start_acceptor, std::ref(a_window), (base_port_trgt + i)).detach();
	}
}

void sigint_handler(int a_signum)
{
	if (a_signum != SIGINT)
	{
		return;
	}
	g_app->quit();
}

int main(int, char const **)
{
	auto app = Gtk::Application::create();
	g_app = app.get();

	std::unique_ptr<UIDialog> dialog = std::make_unique<UIDialog>();
	do
	{
		if (RESPONSE_ID_OK != dialog->run())
		{
			return EXIT_FAILURE;
		}
	} while (!dialog->is_valid());

	signal(SIGINT, sigint_handler);

	UIWindow window{dialog->num_processes()};
	window.signal_delete_event().connect(sigc::mem_fun(window, &UIWindow::on_delete));

	start_servers(window);

	bool ret = false;
	if (dialog->srun())
	{
		ret = start_clients_srun(*dialog);
	}
	else
	{
		ret = start_clients_mpi(dialog->num_processes(), dialog->target()) > 0;
	}

	if (!ret)
	{
		return EXIT_FAILURE;
	}

	dialog.reset();
	window.init();
	app->run(window);

	return EXIT_SUCCESS;
}