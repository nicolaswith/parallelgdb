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

#ifndef MASTER_HPP
#define MASTER_HPP

#include "asio.hpp"
#include <gtkmm.h>

class UIWindow;
class StartupDialog;
struct ssh_session_struct;
typedef struct ssh_session_struct *ssh_session;

class Master
{
	const int m_max_length;
	const int m_base_port_gdb;
	const int m_base_port_trgt;

	Glib::RefPtr<Gtk::Application> m_app;
	UIWindow *m_window;
	StartupDialog *m_dialog;

private:
	int run_cmd(ssh_session &session);
	bool start_slaves_ssh();
	bool start_slaves_local();
	void process_session(asio::ip::tcp::socket socket,
						 const asio::ip::port_type port);
	void start_acceptor(const asio::ip::port_type port);

public:
	Master();
	~Master();

	bool run_startup_dialog();
	void start_servers();
	bool start_slaves();
	void start_GUI();
};

void sigint_handler(int signum);

#endif /* MASTER_HPP */