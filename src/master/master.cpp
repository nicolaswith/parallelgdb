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
#include <regex>
#include <string.h>
#include <libssh/libssh.h>

#include "master.hpp"
#include "window.hpp"
#include "startup.hpp"

using asio::ip::tcp;
using std::string;

static Gtk::Application *s_app;

Master::Master()
	: m_max_length(0x2000), // socat default
	  m_base_port_gdb(0x8000),
	  m_base_port_trgt(0xC000)
{
	m_app = Gtk::Application::create();
	s_app = m_app.get();
}

Master::~Master()
{
	m_app.reset();
	delete m_window;
}

int Master::run_cmd(ssh_session &session)
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

	string cmd = m_dialog->get_cmd();
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

bool Master::start_slaves_ssh()
{
	ssh_session session;
	int rc;

	session = ssh_new();
	if (session == NULL)
	{
		return false;
	}

	ssh_options_set(session, SSH_OPTIONS_HOST, m_dialog->ssh_address());
	ssh_options_set(session, SSH_OPTIONS_USER, m_dialog->ssh_user());

	rc = ssh_connect(session);
	if (rc != SSH_OK)
	{
		fprintf(stderr, "Error connecting to %s: %s\n", m_dialog->ssh_address(), ssh_get_error(session));
		return false;
	}

	if (SSH_KNOWN_HOSTS_OK != ssh_session_is_known_server(session))
	{
		fprintf(stderr, "The server is not valid.\n");
		ssh_disconnect(session);
		ssh_free(session);
		return false;
	}

	rc = ssh_userauth_password(session, NULL, m_dialog->ssh_password());
	if (rc != SSH_AUTH_SUCCESS)
	{
		fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(session));
		ssh_disconnect(session);
		ssh_free(session);
		return false;
	}

	rc = run_cmd(session);
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

bool Master::start_slaves_local()
{
	const int pid = fork();
	if (0 == pid)
	{
		string cmd = m_dialog->get_cmd();
		cmd = std::regex_replace(cmd, std::regex("^ +"), "");
		cmd = std::regex_replace(cmd, std::regex(" +$"), "");
		cmd = std::regex_replace(cmd, std::regex(" +"), " ");
		const int num_spaces = std::count_if(cmd.begin(), cmd.end(),
											 [](char c)
											 { return c == ' '; });
		char **argv = new char *[num_spaces + 1];
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
	return pid > 0;
}

void Master::process_session(tcp::socket socket, const asio::ip::port_type port)
{
	const int rank = UIWindow::get_rank(port);
	mi_h *gdb_handle = nullptr;
	if (UIWindow::src_is_gdb(port))
	{
		m_window->set_conns_gdb(rank, &socket);
		m_window->set_conns_open_gdb(rank, true);
		gdb_handle = mi_alloc_h();
		gdb_handle->line = nullptr;
	}
	else
	{
		m_window->set_conns_trgt(rank, &socket);
	}

	char *data = new char[m_max_length + 8];
	if (nullptr == data)
	{
		throw std::bad_alloc();
	}
	for (;;)
	{
		asio::error_code error;
		const size_t length = socket.read_some(asio::buffer(data, m_max_length), error);
		if (asio::error::eof == error)
		{
			if (UIWindow::src_is_gdb(port))
			{
				m_window->set_conns_open_gdb(rank, false);
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
					*m_window,
					&UIWindow::print_data),
				gdb_handle,
				strdup(data),
				port));
	}
	delete[] data;
}

void Master::start_acceptor(const asio::ip::port_type port)
{
	asio::io_context io_context;
	tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
	process_session(acceptor.accept(), port);
}

void Master::start_servers()
{
	m_window = new UIWindow{m_dialog->num_processes()};

	for (int rank = 0; rank < m_window->num_processes(); ++rank)
	{
		std::thread(&Master::start_acceptor, this, (m_base_port_gdb + rank)).detach();
		std::thread(&Master::start_acceptor, this, (m_base_port_trgt + rank)).detach();
	}
}

bool Master::run_startup_dialog()
{
	m_dialog = new StartupDialog();
	for (;;)
	{
		const int res = m_dialog->run();
		if (Gtk::RESPONSE_OK == res && m_dialog->is_valid())
		{
			return true;
		}
		else if (Gtk::RESPONSE_DELETE_EVENT == res)
		{
			return false;
		}
	}
	return false;
}

bool Master::start_slaves()
{
	bool ret = false;
	if (m_dialog->ssh())
	{
		ret = start_slaves_ssh();
	}
	else
	{
		ret = start_slaves_local();
	}
	delete m_dialog;
	return ret;
}

void Master::start_GUI()
{
	if (!m_window->init(m_app))
	{
		return;
	}
	m_app->run(*m_window->root_window());
}

void sigint_handler(int signum)
{
	if (signum != SIGINT)
	{
		return;
	}
	s_app->quit();
}

int main(int, char const **)
{
	Master master;
	if (!master.run_startup_dialog())
	{
		return EXIT_SUCCESS;
	}
	signal(SIGINT, sigint_handler);

	master.start_servers();
	if (!master.start_slaves())
	{
		fprintf(stderr, "Could not start slaves.\n");
		return EXIT_FAILURE;
	}
	master.start_GUI();

	return EXIT_SUCCESS;
}