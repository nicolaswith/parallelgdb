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
 * @file slave.hpp
 *
 * @brief Header file for the Slave class.
 *
 * This is the header file for the Slave class.
 */

#ifndef SLAVE_HPP
#define SLAVE_HPP

#include <iosfwd>

/// Holds the state of the slave program.
/**
 * This class holds the state of the slave program. It contains all utility
 * to analyze the configuration, start the socat and GDB instances, and then
 * monitor them.
 */
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

	std::string m_tty_gdb;
	std::string m_tty_trgt;

	/// Waits for the socat instances to start.
	bool wait_for_socat() const;
	/// Starts the GDB instance.
	int start_gdb() const;
	/// Starts a socat instance.
	int start_socat(const std::string &tty_name, const int port) const;

public:
	/// Default constructor.
	Slave(const int argc, char **argv);
	/// Destructor.
	~Slave();

	/// Parses the command line arguments.
	bool parse_cl_args();
	/// Obtains the rank assigned to this process.
	bool set_rank();
	/// Generates the name for the PTYs and starts GDB and socat.
	bool start_processes();
	/// Monitors the socat and GDB instances.
	void monitor_processes() const;
	/// Kills all children that were successfully started.
	void kill_children() const;

	/// Prints the help text.
	static void print_help();
};

#endif /* SLAVE_HPP */