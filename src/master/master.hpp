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
 * @file master.hpp
 *
 * @brief Header file for the Master class.
 *
 * This file is the header file for the Master class.
 */

#ifndef MASTER_HPP
#define MASTER_HPP

#include "asio.hpp"
#include <gtkmm.h>

class UIWindow;
class StartupDialog;
#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct ssh_session_struct;
typedef struct ssh_session_struct *ssh_session;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/// Holds the state of the master program.
/**
 * This class holds the state of the master program. It contains the utility to
 * start the master and slave program. Additionally it handles the TCP
 * connections between the master and socat.
 */
class Master
{
	const int m_max_length;
	const int m_base_port_gdb;
	const int m_base_port_trgt;

	Glib::RefPtr<Gtk::Application> m_app;
	UIWindow *m_window;
	StartupDialog *m_dialog;

private:
	/// Executes the launcher command on the remote server.
	int run_cmd(ssh_session &session);
	/// Establishes a SSH connection to the remote server.
	bool start_slaves_ssh();
	/// Starts the slave instances on the local (host) machine.
	bool start_slaves_local();
	/// Handles the TCP communication between socat and the master.
	void process_session(asio::ip::tcp::socket socket,
						 const asio::ip::port_type port);
	/// Waits (blocking) for a TCP connection on the TCP @p port.
	void start_acceptor(asio::ip::tcp::acceptor acceptor,
						const asio::ip::port_type port);

public:
	/// Default constructor.
	Master();
	/// Destructor.
	~Master();

	/// Runs the startup dialog.
	bool run_startup_dialog();
	/// Creates threads for each blocking TCP acceptor call.
	bool start_servers();
	/// Starts the slaves on the desired debug platform.
	bool start_slaves();
	/// Starts the GUI main window.
	void start_GUI();
};

/// Handles the SIGINT signal.
void sigint_handler(int signum);

#endif /* MASTER_HPP */