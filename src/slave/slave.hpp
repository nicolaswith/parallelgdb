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

#include <iosfwd>

class Slave
{
	const int m_argc;
	char **m_argv;
	int m_args_offset;

	char *m_ip_addr;
	char *m_target;
	char *m_rank_str;
	char *m_env_str;

	int m_rank;

	int m_base_port_gdb;
	int m_base_port_trgt;

	int m_pid_socat_gdb;
	int m_pid_socat_trgt;
	int m_pid_gdb;

	bool wait_for_socat() const;
	int start_gdb(const std::string &tty_gdb, const std::string &tty_trgt) const;
	int start_socat(const std::string &tty_name, const int port) const;

public:
	Slave(const int argc, char **argv);
	~Slave();

	bool parse_cl_args();
	bool set_rank();
	bool start_processes();
	void monitor_processes() const;

	static void print_help();
};
