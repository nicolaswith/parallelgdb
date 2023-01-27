/*
	This file is part of ParallelGDB.

	Copyright (c) 2023 by Nicolas With

	ParallelGDB is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ParallelGDB is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ParallelGDB.  If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.
*/

#include <string>
#include <string.h>
#include <libssh/libssh.h>

#include "master.hpp"
#include "window.hpp"
#include "startup.hpp"

using std::string;
using asio::ip::tcp;

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

	string cmd = dialog.get_cmd();
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

bool start_slaves_ssh(StartupDialog &dialog)
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
		fprintf(stderr, "Error starting slaves: %s\n", ssh_get_error(session));
		ssh_disconnect(session);
		ssh_free(session);
		return false;
	}

	ssh_disconnect(session);
	ssh_free(session);

	return true;
}

int start_slaves_local(StartupDialog &dialog)
{
	const int pid = fork();
	if (0 == pid)
	{
		char **argv = new char *[20];
		string cmd = dialog.get_cmd();
		int idx = 0;
		std::istringstream iss(cmd);
		string opt;
		while (std::getline(iss, opt, ' '))
		{
			argv[idx++] = strdup(opt.c_str());
		}
		argv[idx] = nullptr;

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

	char *data = new char[max_length + 8];
	if (nullptr == data)
	{
		throw std::bad_alloc();
	}
	for (;;)
	{
		asio::error_code error;
		const size_t length = socket.read_some(asio::buffer(data, max_length), error);
		if (asio::error::eof == error)
		{
			if (UIWindow::src_is_gdb(port))
			{
				window.set_conns_open_gdb(rank, false);
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

	std::unique_ptr<StartupDialog> dialog = std::make_unique<StartupDialog>();
	do
	{
		if (Gtk::RESPONSE_OK != dialog->run())
		{
			return EXIT_SUCCESS;
		}
	} while (!dialog->is_valid());

	signal(SIGINT, sigint_handler);
	UIWindow window{dialog->num_processes()};
	start_servers(window);

	bool ret = false;
	if (dialog->ssh())
	{
		ret = start_slaves_ssh(*dialog);
	}
	else
	{
		ret = start_slaves_local(*dialog) > 0;
	}
	if (!ret)
	{
		return EXIT_FAILURE;
	}

	dialog.reset();
	if (!window.init(app))
	{
		return EXIT_FAILURE;
	}
	app->run(*window.root_window());

	return EXIT_SUCCESS;
}