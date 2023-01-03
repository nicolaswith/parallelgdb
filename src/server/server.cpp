#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.hpp"

const int max_length = 0x2000; // socat default
const asio::ip::port_type base_port_gdb = 0x8000;
const asio::ip::port_type base_port_trgt = 0xC000;

static Gtk::Application *g_app;

int run_cmd(ssh_session &session, const StartupDialog &dialog)
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

	cmd += " --nodes=";
	cmd += std::to_string(dialog.num_nodes());

	cmd += " --ntasks=";
	cmd += std::to_string(dialog.num_tasks());

	cmd += " --partition=";
	cmd += dialog.partition();

	cmd += " --mpi=";
	cmd += "pmi2";

	cmd += " ";
	cmd += dialog.client();

	cmd += " -s ";
	cmd += dialog.socat();

	cmd += " -g ";
	cmd += dialog.gdb();

	cmd += " -i ";
	cmd += dialog.ip_address();

	cmd += " ";
	cmd += dialog.target();

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

bool start_clients_srun(StartupDialog &dialog)
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

int start_clients_mpi(StartupDialog &dialog)
{
	const int pid = fork();
	if (0 == pid)
	{
		const char *const np_str = strdup(std::to_string(dialog.num_processes()).c_str());

		char *argv[] = {
			(char *)"/usr/bin/mpirun",
			(char *)"-np",
			(char *)np_str,
			(char *)dialog.client(),
			(char *)"-s",
			(char *)dialog.socat(),
			(char *)"-g",
			(char *)dialog.gdb(),
			(char *)"-i",
			(char *)dialog.ip_address(),
			(char *)dialog.target(),
			(char *)nullptr};
		execvp(argv[0], argv);
		_exit(127);
	}
	return pid;
}

void process_session(tcp::socket socket, UIWindow &window, const int port)
{
	const int rank = UIWindow::get_rank(port);
	mi_h *gdb_handle = nullptr;
	if (UIWindow::src_is_gdb(port))
	{
		window.set_conns_gdb(rank, &socket);
		window.set_conns_open_gdb(rank, true);
		gdb_handle = mi_alloc_h();
		gdb_handle->line = nullptr;
	}
	else
	{
		window.set_conns_trgt(rank, &socket);
	}

	try
	{
		char *data = new char[max_length + 8];
		for (;;)
		{
			if (nullptr == data)
			{
				throw std::bad_alloc();
			}
			asio::error_code error;
			const size_t length = socket.read_some(asio::buffer(data, max_length), error);
			if (asio::error::eof == error)
			{
				if (UIWindow::src_is_gdb(port))
				{
					window.set_conns_open_gdb(rank, false);
					delete[] data;
					mi_free_h(&gdb_handle);
				}
				break;
			}
			else if (error)
			{
				throw asio::system_error(error);
			}
			// add null termination to received data.
			data[length] = '\0';
			Glib::signal_idle().connect_once(
				sigc::bind(
					sigc::mem_fun(
						window,
						&UIWindow::print_data),
					gdb_handle,
					strdup(data),
					port));
		}
		delete[] data;
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception in thread: " << e.what() << "\n";
	}
}

void start_acceptor(UIWindow &window, const asio::ip::port_type port)
{
	asio::io_context io_context;
	tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
	process_session(acceptor.accept(), window, port);
}

void start_servers(UIWindow &window)
{
	for (int rank = 0; rank < window.num_processes(); ++rank)
	{
		std::thread(start_acceptor, std::ref(window), (base_port_gdb + rank)).detach();
		std::thread(start_acceptor, std::ref(window), (base_port_trgt + rank)).detach();
	}
}

void sigint_handler(int signum)
{
	if (signum != SIGINT)
	{
		return;
	}
	g_app->quit();
}

int main(int, char const **)
{
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create();
	g_app = app.get();
	set_application(app.get());

	std::unique_ptr<StartupDialog> dialog = std::make_unique<StartupDialog>();
	do
	{
		if (Gtk::RESPONSE_OK != dialog->run())
		{
			return EXIT_FAILURE;
		}
	} while (!dialog->is_valid());

	signal(SIGINT, sigint_handler);
	UIWindow window{dialog->num_processes()};
	start_servers(window);

	bool ret = false;
	if (dialog->srun())
	{
		ret = start_clients_srun(*dialog);
	}
	else
	{
		ret = start_clients_mpi(*dialog) > 0;
	}
	if (!ret)
	{
		return EXIT_FAILURE;
	}

	dialog.reset();
	if (!window.init())
	{
		return EXIT_FAILURE;
	}
	app->run(*window.root_window());

	return EXIT_SUCCESS;
}