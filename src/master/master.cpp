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
	along with ParallelGDB.  If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file master.cpp
 *
 * @brief Contains the implementation of the Master class.
 *
 * This file contains the implementation of the Master class.
 */

#include <exception>
#include <string>
#include <string.h>
#include <algorithm>
#include <utility>
#include <unistd.h>
#include <cstdio>

#include "master.hpp"
#include "window.hpp"
#include "startup.hpp"

using asio::ip::tcp;
using std::string;

#define MAX_LENGTH 8192 // socat default

static Gtk::Application *s_app;

/**
 * This is the default constructor for the Master class. It sets the base ports
 * for the TCP sockets and creates a new Gtk::Application, which will later
 * display the GUI.
 */
Master::Master()
{
	m_app = Gtk::Application::create();
	s_app = m_app.get();
}

/**
 * This function will delete the app and the GUI window.
 */
Master::~Master()
{
	m_app.reset();
	delete m_window;
}

/**
 * This function executes the launcher command on the remote server.
 *
 * @param[in] session The SSH session.
 *
 * @return @c SSH_OK on success, @c SSH_ERROR on error.
 */
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
	printf("Started slaves via SSH with command:\n%s\n", cmd.c_str());

	ssh_channel_send_eof(channel);
	ssh_channel_close(channel);
	ssh_channel_free(channel);

	return SSH_OK;
}

/**
 * This function establishes a SSH connection to the remote server using the
 * username and password set in the startup dialog.
 *
 * @return @c true on success, @c false on error.
 */
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
		fprintf(stderr, "Error connecting to %s: %s\n", m_dialog->ssh_address(),
				ssh_get_error(session));
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
		fprintf(stderr, "Error authenticating with password: %s\n",
				ssh_get_error(session));
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

/**
 * This function starts the slave instances on the local (host) machine.
 *
 * @return The forked PID on success, @c -1 on error.
 */
bool Master::start_slaves_local()
{
	const int pid = fork();
	if (0 == pid)
	{
		// close all file descriptors except stdin, stdout and stderr
		int fd_limit = (int)sysconf(_SC_OPEN_MAX);
		for (int fd = STDERR_FILENO + 1; fd < fd_limit; fd++)
		{
			close(fd);
		}

		string cmd = m_dialog->get_cmd();
		printf("Started slaves with command:\n%s\n", cmd.c_str());
		const int num_spaces =
			std::count_if(cmd.begin(), cmd.end(),
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

/**
 * This function handles the TCP communication between socat and the master.
 * It waits (blocking) for data. When data is received, a copy is sent to be
 * displayed/parsed. On error the connection is closed and the master
 * terminates.
 *
 * @param socket The TCP socket.
 *
 * @param port The assigned port of the @p socket.
 */
void Master::read_data(tcp::socket socket, const asio::ip::port_type port)
{
	const int rank = m_window->get_rank(port);
	if (m_window->src_is_gdb(port))
	{
		m_window->set_conns_gdb(rank, &socket);
	}
	else
	{
		m_window->set_conns_trgt(rank, &socket);
	}

	// Allocate enough memory for a socat message. This memory will be
	// overwritten without clearing it, as it is '\0'-terminated anyway.
	char *data = new char[MAX_LENGTH + 8];
	if (nullptr == data)
	{
		throw std::bad_alloc();
	}
	for (;;)
	{
		asio::error_code error;
		const size_t length =
			socket.read_some(asio::buffer(data, MAX_LENGTH), error);
		if (asio::error::eof == error)
		{
			if (m_window->src_is_gdb(port))
			{
				m_window->set_conns_gdb(rank, nullptr);
			}
			else
			{
				m_window->set_conns_trgt(rank, nullptr);
			}
			// there should be no data on eof ... ?
			break;
		}
		else if (error)
		{
			throw asio::system_error(error);
		}
		// add null termination to received data.
		data[length] = '\0';
		// hand a copy of the data to the print function
		Glib::signal_idle().connect_once(
			sigc::bind(sigc::mem_fun(*m_window, &UIWindow::handle_data),
					   strdup(data), port));
	}
	delete[] data;
}

/**
 * This function waits for a TCP connection on the TCP @p port.
 *
 * @param acceptor The asio acceptor bound to the TCP address and port.
 *
 * @param port The assigned port of the @p socket.
 */
void Master::start_acceptor(tcp::acceptor acceptor,
							const asio::ip::port_type port)
{
	tcp::socket socket(acceptor.accept());
	acceptor.close();
	read_data(std::move(socket), port);
}

/**
 * This function creates threads for each blocking TCP acceptor call. It checks
 * for port collision when creating the acceptor.
 *
 * @return @c true when all acceptors could be created, @c false on
 * port collision.
 */
bool Master::start_servers()
{
	m_window = new UIWindow{m_dialog->num_processes(), m_dialog->base_port()};

	for (int offset = 0; offset < 2 * m_dialog->num_processes(); ++offset)
	{
		const int port = m_dialog->base_port() + offset;
		try
		{
			std::thread(&Master::start_acceptor,
						this,
						tcp::acceptor(
							*(new asio::io_context),
							tcp::endpoint(tcp::v4(), port)),
						port)
				.detach();
		}
		catch (const std::exception &)
		{
			fprintf(stderr,
					"Error: TCP port %d needed but already in use.\n",
					port);
			return false;
		}
	}

	return true;
}

/**
 * This function runs the startup dialog.
 *
 * @return @c true if the master application should start, @c false if the
 * dialog was closed and the master should terminate.
 */
bool Master::run_startup_dialog()
{
	m_dialog = new StartupDialog;
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

/**
 * This function starts the slaves on the desired debug platform.
 *
 * @note
 * The master does not check for successful launch of the slaves, as this
 * happens after the fork / on the remote server. Success in this context
 * means only that the fork was successful / the SSH connection could be
 * established and the launch command was sent.
 *
 * @return @c true on success, @c false on error.
 */
bool Master::start_slaves()
{
	bool ret = true;
	if (m_dialog->master_starts_slaves())
	{
		if (m_dialog->ssh())
		{
			ret = start_slaves_ssh();
		}
		else
		{
			ret = start_slaves_local();
		}
	}
	delete m_dialog;
	return ret;
}

/**
 * This function starts the GUI main window.
 */
void Master::start_GUI()
{
	if (!m_window->init(m_app))
	{
		return;
	}
	m_app->run(*m_window->root_window());
}

/**
 * This function handles the SIGINT signal. It will close the master and thus
 * the slave program.
 *
 * @param signum The signal number.
 */
void sigint_handler(int signum)
{
	if (signum != SIGINT)
	{
		return;
	}
	s_app->quit();
}

/// Master entry point.
/**
 * This is the entry point for ParallelGDB. It will first open a dialog to
 * configure the necessary parameters and then start the slaves and the GUI.
 */
int main(int, char const **)
{
	Master master;
	if (!master.run_startup_dialog())
	{
		return EXIT_SUCCESS;
	}
	signal(SIGINT, sigint_handler);

	if (!master.start_servers())
	{
		return EXIT_FAILURE;
	}
	if (!master.start_slaves())
	{
		fprintf(stderr, "Could not start slaves.\n");
		return EXIT_FAILURE;
	}
	master.start_GUI();

	return EXIT_SUCCESS;
}