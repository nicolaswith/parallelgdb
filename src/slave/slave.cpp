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

#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fcntl.h>
#include <sys/wait.h>

#include "slave.hpp"

using namespace std;

Slave::Slave(const int argc, char **argv)
	: m_argc(argc),
	  m_argv(argv),
	  m_args_offset(-1),
	  m_ip_addr(nullptr),
	  m_target(nullptr),
	  m_rank_str(nullptr),
	  m_env_str(nullptr),
	  m_rank(-1),
	  m_pid_socat_gdb(-1),
	  m_pid_socat_trgt(-1),
	  m_pid_gdb(-1)
{
	m_base_port_gdb = 0x8000;
	m_base_port_trgt = 0xC000;
}

Slave::~Slave()
{
	free(m_target);
	free(m_ip_addr);
	free(m_rank_str);
	free(m_env_str);
}

bool Slave::wait_for_socat() const
{
	int exited = 0;
	while (0 == exited)
	{
		exited = 0;
		exited |= waitpid(m_pid_socat_gdb, nullptr, WNOHANG);
		exited |= waitpid(m_pid_socat_trgt, nullptr, WNOHANG);
		FILE *cmd = popen("pidof socat", "r");
		char result[1024] = {0};
		bool found_gdb = false;
		bool found_trgt = false;
		while (fgets(result, sizeof(result), cmd) != NULL)
		{
			int pid;
			istringstream stream(result);
			while (stream >> pid)
			{
				if (pid == m_pid_socat_gdb)
				{
					found_gdb = true;
				}
				if (pid == m_pid_socat_trgt)
				{
					found_trgt = true;
				}
			}
			if (found_gdb && found_trgt)
			{
				return true;
			}
		}
		pclose(cmd);
		usleep(100000);
	}
	return false;
}

int Slave::start_gdb(const string &tty_gdb, const string &tty_trgt) const
{
	if (!wait_for_socat())
	{
		return -1;
	}
	usleep(500000);

	int pid = fork();
	if (0 == pid)
	{
		int tty_fd = open(tty_gdb.c_str(), O_RDWR);

		dup2(tty_fd, STDIN_FILENO);
		dup2(tty_fd, STDOUT_FILENO);
		dup2(tty_fd, STDERR_FILENO);

		string tty = "--tty=" + tty_trgt;

		const int num_args = m_argc - m_args_offset;
		char **argv_gdb = (char **)malloc((10 + num_args) * sizeof(char *));

		int idx = 0;
		argv_gdb[idx++] = (char *)"gdb";
		argv_gdb[idx++] = (char *)"-q";
		argv_gdb[idx++] = (char *)"-i";
		argv_gdb[idx++] = (char *)"mi3";
		argv_gdb[idx++] = (char *)"-ex=set auto-load safe-path /";
		argv_gdb[idx++] = (char *)"-ex=b main";
		argv_gdb[idx++] = (char *)tty.c_str();
		if (0 != num_args)
		{
			argv_gdb[idx++] = (char *)"--args";
		}
		argv_gdb[idx++] = (char *)m_target;
		for (int i = 0; i < num_args; ++i)
		{
			argv_gdb[idx++] = (char *)m_argv[m_args_offset + i];
		}
		argv_gdb[idx] = (char *)nullptr;

		execvp(argv_gdb[0], argv_gdb);
		fprintf(stderr, "Error starting gdb. %s\n", strerror(errno));
		_exit(127);
	}

	return pid;
}

int Slave::start_socat(const string &tty_name, const int port) const
{
	int pid = fork();
	if (0 == pid)
	{
		string tty = "pty,echo=0,link=" + tty_name;
		string tcp = "TCP:" + string(m_ip_addr) + ":" + to_string(port);

		char *argv[] = {
			(char *)"socat",
			(char *)tty.c_str(),
			(char *)tcp.c_str(),
			(char *)nullptr};
		execvp(argv[0], argv);
		fprintf(stderr, "Error starting socat (%s, rank: %d). %s\n", (0 == (port & 0x4000)) ? "gdb" : "target", port & 0x3FFF, strerror(errno));
		_exit(127);
	}
	return pid;
}

bool Slave::parse_cl_args()
{
	char c;
	opterr = 0;
	while ((c = getopt(m_argc, m_argv, "hi:r:e:")) != -1)
	{
		switch (c)
		{
		case 'i': // ip
			free(m_ip_addr);
			m_ip_addr = strdup(optarg);
			break;
		case 'r': // rank
			free(m_rank_str);
			m_rank_str = strdup(optarg);
			break;
		case 'e': // env var name
			free(m_env_str);
			m_env_str = strdup(optarg);
			break;
		case 'h': // help
			print_help();
			exit(EXIT_SUCCESS);
			break;
		case '?':
			if ('i' == optopt)
			{
				fprintf(stderr, "Option -%c requires the host IP address.\n", optopt);
			}
			else if ('r' == optopt)
			{
				fprintf(stderr, "Option -%c requires the process rank.\n", optopt);
			}
			else if ('e' == optopt)
			{
				fprintf(stderr, "Option -%c requires the name for the environment variable containing the process rank.\n", optopt);
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
	m_args_offset = optind + 1;
	return true;
}

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
		if (m_env_str)
		{
			env_vars.insert(env_vars.begin(), m_env_str);
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
		m_rank = stoi(rank);
	}
	catch (const exception &e)
	{
		fprintf(stderr, "Could not parse rank to integer. String: %s\n", rank);
		print_help();
		return false;
	}
	return true;
}

bool Slave::start_processes()
{
	ostringstream tty_gdb_oss;
	ostringstream tty_trgt_oss;
	tty_gdb_oss << "/tmp/ttyGDB_" << setw(4) << setfill('0') << to_string(m_rank);
	tty_trgt_oss << "/tmp/ttyTRGT_" << setw(4) << setfill('0') << to_string(m_rank);
	string tty_gdb = tty_gdb_oss.str();
	string tty_trgt = tty_trgt_oss.str();

	const int port_gdb = m_base_port_gdb + m_rank;
	const int port_trgt = m_base_port_trgt + m_rank;

	m_pid_socat_gdb = start_socat(tty_gdb, port_gdb);
	m_pid_socat_trgt = start_socat(tty_trgt, port_trgt);

	if (m_pid_socat_gdb > 0 && m_pid_socat_trgt > 0)
	{
		m_pid_gdb = start_gdb(tty_gdb, tty_trgt);
	}

	return (m_pid_gdb > 0) && (m_pid_socat_gdb > 0) && (m_pid_socat_trgt > 0);
}

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

	if (m_pid_gdb > 0)
	{
		kill(m_pid_gdb, SIGINT);
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

void Slave::print_help()
{
	fprintf(
		stderr,
		"Usage: ./pgdbslave -i <addr> [OPTIONS] </path/to/target>\n"
		"  -i <addr>\t host IP address\n"
		"  -h\t\t print this help\n"
		"\n"
		"Options:\n"
		"Only needed when using custom launcher command and only one at a time:\n"
		"  -r <rank>\t rank of process\n"
		"  -e <name>\t name of the environment variable containing the process rank\n");
}

int main(const int argc, char **argv)
{
	Slave slave{argc, argv};

	if (!slave.parse_cl_args())
	{
		return EXIT_FAILURE;
	}
	if (!slave.set_rank())
	{
		return EXIT_FAILURE;
	}
	if (!slave.start_processes())
	{
		return EXIT_FAILURE;
	}
	slave.monitor_processes();

	return EXIT_SUCCESS;
}