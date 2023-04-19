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
 * @file slave.cpp
 *
 * @brief Contains the implementation of the Slave class.
 *
 * This file contains the implementation of the Slave class.
 */

#include <exception>
#include <string.h>
#include <unistd.h>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fcntl.h>
#include <sys/wait.h>
#include <filesystem>

#include "slave.hpp"

using namespace std;

/**
 * This is the default constructor for the Slave class.
 *
 * @param argc The total number of arguments passed to this program.
 *
 * @param[in] argv The array containing the arguments passed to this program.
 * This NEEDS to be null-terminated.
 */
Slave::Slave(const int argc, char **argv)
	: m_argc(argc),
	  m_argv(argv),
	  m_args_offset(-1),
	  m_ip_addr(nullptr),
	  m_base_port_str(nullptr),
	  m_target(nullptr),
	  m_rank_str(nullptr),
	  m_rank_env_str(nullptr),
	  m_size_str(nullptr),
	  m_size_env_str(nullptr),
	  m_rank(-1),
	  m_size(-1),
	  m_base_port(-1),
	  m_pid_socat_gdb(-1),
	  m_pid_socat_trgt(-1),
	  m_pid_gdb(-1)
{
}

/**
 * This function frees the allocated char arrays.
 */
Slave::~Slave()
{
	free(m_target);
	free(m_ip_addr);
	free(m_rank_str);
	free(m_rank_env_str);
	free(m_size_str);
	free(m_size_env_str);
}

/**
 * This function waits for the socat instances to start. It checks that both
 * instances are alive and whether the PTY has been created. This is only done
 * when the TCP connection is established, so everything is ready for GDB to
 * start.
 *
 * @return @c true on success, @c false when either of the socat instances dies.
 */
bool Slave::wait_for_socat() const
{
	int exited = 0;
	while (0 == exited)
	{
		exited = 0;
		exited |= waitpid(m_pid_socat_gdb, nullptr, WNOHANG);
		exited |= waitpid(m_pid_socat_trgt, nullptr, WNOHANG);
		const bool found_gdb = filesystem::exists(m_tty_gdb);
		const bool found_trgt = filesystem::exists(m_tty_trgt);
		if (found_gdb && found_trgt)
		{
			return true;
		}
		usleep(1000);
	}
	return false;
}

/**
 * This function starts the GDB instance. It ensures both socat instances are
 * running, by calling the @ref wait_for_socat function before forking. The I/O
 * of the new process is connected to the PTY created by socat. The target I/O
 * will be connected to the second PTY created by the other socat instance. This
 * is done by GBD with the --tty option. The user arguments are forwarded to
 * the target program.
 *
 * @return The PID of the forked process, or @c -1 on error.
 */
int Slave::start_gdb() const
{
	if (!wait_for_socat())
	{
		return -1;
	}

	const int pid = fork();
	if (0 == pid)
	{
		char *pts = strdup(ttyname(STDOUT_FILENO));
		const int tty_fd = open(m_tty_gdb.c_str(), O_RDWR);

		// connect I/O of GDB to PTY
		dup2(tty_fd, STDIN_FILENO);
		dup2(tty_fd, STDOUT_FILENO);
		dup2(tty_fd, STDERR_FILENO);

		// prepare command to instruct GDB to send the target output on this PTY
		const string tty = "--tty=" + m_tty_trgt;

		const int num_args = m_argc - m_args_offset;
		char **argv_gdb = new char *[11 + num_args];

		int idx = 0;
		argv_gdb[idx++] = (char *)"gdb";
		argv_gdb[idx++] = (char *)"-q";
		argv_gdb[idx++] = (char *)"-i";
		argv_gdb[idx++] = (char *)"mi3";
		argv_gdb[idx++] = (char *)tty.c_str();
		if (0 != num_args)
		{
			argv_gdb[idx++] = (char *)"--args";
		}
		argv_gdb[idx++] = (char *)"-ex=set auto-load safe-path /";
		argv_gdb[idx++] = (char *)"-ex=break main";
		argv_gdb[idx++] = (char *)"-ex=run";
		argv_gdb[idx++] = (char *)m_target;
		// append user arguments
		for (int i = 0; i < num_args; ++i)
		{
			argv_gdb[idx++] = (char *)m_argv[m_args_offset + i];
		}
		argv_gdb[idx] = (char *)nullptr;

		execvp(argv_gdb[0], argv_gdb);

		// Only reached if exec fails
		close(tty_fd);

		const int std_fd = open(pts, O_RDWR);
		dup2(std_fd, STDIN_FILENO);
		dup2(std_fd, STDOUT_FILENO);
		dup2(std_fd, STDERR_FILENO);

		free(pts);

		fprintf(stderr,
				"Error starting gdb.\n"
				"\tRank: %d\n"
				"\tError message: %s\n",
				m_rank, strerror(errno));
		_exit(127);
	}

	return pid;
}

/**
 * This function starts a socat instance. This is done by forking and then
 * calling exec to replace the process image.
 *
 * socat will create a temporal PTY, to which the GDB instance is connected to
 * in the @ref start_gdb function.
 *
 * @param[in] tty_name The name of the PTY to be created.
 *
 * @param port The designated TCP port at the master for this process.
 *
 * @return The PID of the forked process, or @c -1 on error.
 *
 * @note
 * This function is called twice. Once to create the PTY for GDB and once
 * for the target program.
 */
int Slave::start_socat(const string &tty_name, const int port) const
{
	const int pid = fork();
	if (0 == pid)
	{
		const string tcp = "TCP:" + string(m_ip_addr) + ":" + to_string(port);
		const string tty = "PTY,echo=0,link=" + tty_name;

		char *argv[] = {
			(char *)"socat",
			(char *)tcp.c_str(),
			(char *)tty.c_str(),
			(char *)nullptr};
		execvp(argv[0], argv);
		// Only reached if exec fails
		fprintf(stderr,
				"Error starting socat.\n"
				"\tRank: %d, TCP: '%s', PTY: '%s'\n"
				"\tError message: %s\n",
				m_rank, tcp.c_str(), tty_name.c_str(), strerror(errno));
		_exit(127);
	}
	return pid;
}

/**
 * This function parses the command line arguments. User arguments are passed
 * after the path to the target.
 *
 * @return @c true on success, @c false on error.
 */
bool Slave::parse_cl_args()
{
	char c;
	opterr = 0;
	while ((c = getopt(m_argc, m_argv, "+hi:p:r:k:s:z:")) != -1)
	{
		switch (c)
		{
		case 'i': // ip
			free(m_ip_addr);
			m_ip_addr = strdup(optarg);
			break;
		case 'p': // port
			free(m_base_port_str);
			m_base_port_str = strdup(optarg);
			break;
		case 'r': // rank
			free(m_rank_str);
			m_rank_str = strdup(optarg);
			break;
		case 'k': // rank env var name
			free(m_rank_env_str);
			m_rank_env_str = strdup(optarg);
			break;
		case 's': // size
			free(m_size_str);
			m_size_str = strdup(optarg);
			break;
		case 'z': // size env var name
			free(m_size_env_str);
			m_size_env_str = strdup(optarg);
			break;
		case 'h': // help
			print_help();
			exit(EXIT_SUCCESS);
			break;
		case '?':
			if ('i' == optopt)
			{
				fprintf(stderr,
						"Option -%c requires the host IP address.\n", optopt);
			}
			else if ('p' == optopt)
			{
				fprintf(stderr,
						"Option -%c requires the base port.\n", optopt);
			}
			else if ('r' == optopt)
			{
				fprintf(stderr,
						"Option -%c requires the process rank.\n", optopt);
			}
			else if ('k' == optopt)
			{
				fprintf(stderr,
						"Option -%c requires the name for the environment "
						"variable containing the process rank.\n",
						optopt);
			}
			else if ('s' == optopt)
			{
				fprintf(stderr,
						"Option -%c requires the number of processes.\n", optopt);
			}
			else if ('z' == optopt)
			{
				fprintf(stderr,
						"Option -%c requires the name for the environment "
						"variable containing the number of processes.\n",
						optopt);
			}
			else if (isprint(optopt))
			{
				fprintf(stderr, "Unknown option '-%c'.\n", optopt);
			}
			else
			{
				fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
			}
			[[fallthrough]];
		default:
			print_help();
			return false;
		}
	}
	if (m_argc > optind)
	{
		m_target = strdup(m_argv[optind]);
	}
	else
	{
		fprintf(stderr, "No target specified.\n");
		print_help();
		return false;
	}
	if (nullptr == m_ip_addr)
	{
		fprintf(stderr, "Missing host IP address.\n");
		print_help();
		return false;
	}
	if (nullptr == m_base_port_str)
	{
		fprintf(stderr, "Missing base port.\n");
		print_help();
		return false;
	}
	// store the offset of the first user argument
	m_args_offset = optind + 1;
	return true;
}

/**
 * This function parses the base port string to an integer.
 *
 * @return @c true on success, @c false on error.
 */
bool Slave::set_base_port()
{
	try
	{
		size_t pos;
		m_base_port = stoi(m_base_port_str, &pos, 10);
		if (pos != strlen(m_base_port_str))
		{
			throw exception();
		}
	}
	catch (const exception &)
	{
		fprintf(stderr,
				"Could not parse base port to integer. String: %s\n",
				m_base_port_str);
		print_help();
		return false;
	}
	return true;
}

/**
 * This function obtains the rank assigned to this process. The rank can be
 * retrievd from the environment variables or the command line arguments. The
 * user can pass a environment variable name with the -e option to the process.
 * This variable will then be parsed for the rank.
 *
 * The priority of the actions is as follows: Rank set via:
 * 1. the -r command line option
 * 2. the custom environment variable (-e)
 * 3. the MPI/slurm default environment variables (default)
 *
 * @return @c true on success, @c false on error.
 */
bool Slave::set_rank()
{
	const char *rank = nullptr;
	if (m_rank_str)
	{
		rank = m_rank_str;
	}
	else
	{
		vector<const char *> env_vars = {
			"OMPI_COMM_WORLD_RANK",
			"PMI_RANK"};
		if (m_rank_env_str)
		{
			// push custom enviornment variable to front, so it is check first
			env_vars.insert(env_vars.begin(), m_rank_env_str);
		}
		for (const char *const env_var : env_vars)
		{
			rank = getenv(env_var);
			if (rank)
			{
				break;
			}
		}
	}
	if (!rank)
	{
		fprintf(stderr, "Could not read environemnt variable containing rank.\n");
		print_help();
		return false;
	}
	try
	{
		size_t pos;
		m_rank = stoi(rank, &pos, 10);
		if (pos != strlen(rank))
		{
			throw exception();
		}
	}
	catch (const exception &)
	{
		fprintf(stderr, "Could not parse rank to integer. String: %s\n", rank);
		print_help();
		return false;
	}
	return true;
}

/**
 * This function obtains the number of started processes.
 *
 * @return @c true on success, @c false on error.
 */
bool Slave::set_size()
{
	const char *size = nullptr;
	if (m_size_str)
	{
		size = m_size_str;
	}
	else
	{
		vector<const char *> env_vars = {
			"OMPI_COMM_WORLD_SIZE",
			"PMI_SIZE"};
		if (m_size_env_str)
		{
			// push custom enviornment variable to front, so it is check first
			env_vars.insert(env_vars.begin(), m_size_env_str);
		}
		for (const char *const env_var : env_vars)
		{
			size = getenv(env_var);
			if (size)
			{
				break;
			}
		}
	}
	if (!size)
	{
		fprintf(stderr, "Could not read environemnt variable containing size.\n");
		print_help();
		return false;
	}
	try
	{
		size_t pos;
		m_size = stoi(size, &pos, 10);
		if (pos != strlen(size))
		{
			throw exception();
		}
	}
	catch (const exception &)
	{
		fprintf(stderr, "Could not parse size to integer. String: %s\n", size);
		print_help();
		return false;
	}
	return true;
}

/**
 * This function generates the name for the PTYs and calculates the designated
 * TCP ports at the master. Then the socat instances are stated and when
 * successful, the GDB instance is started.
 *
 * @return @c true when all three instances are running, @c false on error.
 */
bool Slave::start_processes()
{
	ostringstream tty_gdb_oss;
	ostringstream tty_trgt_oss;
	tty_gdb_oss << "/tmp/ttyGDB"
				<< setw(4) << setfill('0') << to_string(m_rank);
	tty_trgt_oss << "/tmp/ttyTRGT"
				 << setw(4) << setfill('0') << to_string(m_rank);
	m_tty_gdb = tty_gdb_oss.str();
	m_tty_trgt = tty_trgt_oss.str();

	const int port_gdb = m_base_port + m_rank;
	const int port_trgt = m_base_port + m_rank + m_size;

	m_pid_socat_gdb = start_socat(m_tty_gdb, port_gdb);
	m_pid_socat_trgt = start_socat(m_tty_trgt, port_trgt);

	if (m_pid_socat_gdb > 0 && m_pid_socat_trgt > 0)
	{
		m_pid_gdb = start_gdb();
	}

	return (m_pid_gdb > 0) && (m_pid_socat_gdb > 0) && (m_pid_socat_trgt > 0);
}

/**
 * This function monitors the socat and GDB instances for exiting. When either
 * of these processes exits, all others are killed as well.
 */
void Slave::monitor_processes() const
{
	int exited = 0;
	while (0 == exited)
	{
		sleep(1);
		exited = 0;
		exited |= waitpid(m_pid_gdb, nullptr, WNOHANG);
		exited |= waitpid(m_pid_socat_gdb, nullptr, WNOHANG);
		exited |= waitpid(m_pid_socat_trgt, nullptr, WNOHANG);
	}
	kill_children();
}

/**
 * This function kills all children (socat 2x, GDB) that were
 * successfully started.
 */
void Slave::kill_children() const
{
	if (m_pid_gdb > 0)
	{
		kill(m_pid_gdb, SIGKILL);
	}
	if (m_pid_socat_gdb > 0)
	{
		kill(m_pid_socat_gdb, SIGINT);
	}
	if (m_pid_socat_trgt > 0)
	{
		kill(m_pid_socat_trgt, SIGINT);
	}
}

/**
 * This function prints the help text.
 */
void Slave::print_help()
{
	fprintf(
		stderr,
		"Usage: ./pgdbslave -i <addr> -p <port> [OPTIONS] </path/to/target>\n"
		"  -i <addr>\t host IP address\n"
		"  -p <port>\t the base port\n"
		"  -h\t\t print this help\n"
		"\n"
		"Options:\n"
		"Only needed when using custom launcher command with unsupported launcher:\n"
		"  -r <rank>\t rank of process\n"
		"  -k <name>\t name of the environment variable containing the process rank\n"
		"  -s <size>\t number of processes\n"
		"  -z <name>\t name of the environment variable containing the number of processes\n");
}

/// Entry point for the slave program.
/**
 * This program starts and monitors the socat and GDB instances.
 *
 * @param argc The number of arguments passed to this program.
 *
 * @param argv The array containing the arguments.
 *
 * @return The exit state of the program.
 */
int main(const int argc, char **argv)
{
	Slave slave(argc, argv);

	if (!slave.parse_cl_args())
	{
		return EXIT_FAILURE;
	}
	if (!slave.set_base_port())
	{
		return EXIT_FAILURE;
	}
	if (!slave.set_rank())
	{
		return EXIT_FAILURE;
	}
	if (!slave.set_size())
	{
		return EXIT_FAILURE;
	}
	if (!slave.start_processes())
	{
		slave.kill_children();
		return EXIT_FAILURE;
	}
	slave.monitor_processes();

	return EXIT_SUCCESS;
}