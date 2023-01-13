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

#ifndef SERVER_HPP
#define SERVER_HPP

#include "defs.hpp"
#include "window.hpp"
#include "startup.hpp"

#include <libssh/libssh.h>

int run_cmd(ssh_session &session, const StartupDialog &dialog);
bool start_clients_ssh(const StartupDialog &dialog);
int start_clients_mpi(const StartupDialog &dialog);
void process_session(tcp::socket socket, UIWindow &window, const int port);
void start_acceptor(UIWindow &window, const asio::ip::port_type port);
void start_servers(UIWindow &window);

#endif /* SERVER_HPP */