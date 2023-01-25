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

using namespace std;

bool wait_for_socat(const int pid_socat_gdb, const int pid_socat_trgt)
{
	for (;;)
	{
		FILE *cmd = popen("pidof socat", "r");
		char result[1024] = {0};
		while (fgets(result, sizeof(result), cmd) != NULL)
		{
			int pid;
			bool found_gdb = false;
			bool found_trgt = false;
			istringstream stream(result);
			while (stream >> pid)
			{
				if (pid == pid_socat_gdb)
				{
					found_gdb = true;
				}
				if (pid == pid_socat_trgt)
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
		usleep(100);
	}
}

int start_gdb(const int argc, char **argv, const int args_offset, const string &tty_gdb, const string &tty_trgt, const char *const target, const int pid_socat_gdb, const int pid_socat_trgt)
{
	if (!wait_for_socat(pid_socat_gdb, pid_socat_trgt))
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

		const int num_args = argc - args_offset;
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
		argv_gdb[idx++] = (char *)target;
		for (int i = 0; i < num_args; ++i)
		{
			argv_gdb[idx++] = (char *)argv[args_offset + i];
		}
		argv_gdb[idx++] = (char *)nullptr;

		execvp(argv_gdb[0], argv_gdb);
		fprintf(stderr, "Error starting gdb. %s\n", strerror(errno));
		_exit(127);
	}

	return pid;
}

int start_socat(const string &tty_name, const char *const ip_addr, const int port)
{
	int pid = fork();
	if (0 == pid)
	{
		string tty = "pty,echo=0,link=" + tty_name;
		string tcp = "TCP:" + string(ip_addr) + ":" + to_string(port);

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

int get_rank(const char *const rank_str, const char *env_str)
{
	const char *rank = nullptr;
	if (rank_str)
	{
		rank = rank_str;
	}
	else
	{
		vector<const char *> env_vars = {
			"OMPI_COMM_WORLD_RANK",
			"PMI_RANK"};
		if (env_str)
		{
			env_vars.insert(env_vars.begin(), env_str);
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
		return -1;
	}
	try
	{
		return stoi(rank);
	}
	catch (const exception &e)
	{
		return -1;
	}
}

void print_help()
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

void free_char_arrays(char *ip_addr, char *target, char *rank_str, char *env_str)
{
	free(target);
	free(ip_addr);
	free(rank_str);
	free(env_str);
}

int parse_cl_args(const int argc, char **argv, char **ip_addr, char **target, char **rank_str, char **env_str)
{
	char c;
	opterr = 0;
	while ((c = getopt(argc, argv, "hi:r:e:")) != -1)
	{
		switch (c)
		{
		case 'i': // ip
			free(*ip_addr);
			*ip_addr = strdup(optarg);
			break;
		case 'r': // rank
			free(*rank_str);
			*rank_str = strdup(optarg);
			break;
		case 'e': // env var name
			free(*env_str);
			*env_str = strdup(optarg);
			break;
		case 'h': // help
			print_help();
			free_char_arrays(*ip_addr, *target, *rank_str, *env_str);
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
			return -1;
		}
	}
	if (argc > optind)
	{
		*target = strdup(argv[optind]);
	}
	else
	{
		fprintf(stderr, "No target specified.\n");
		print_help();
		return -1;
	}
	if (nullptr == ip_addr)
	{
		fprintf(stderr, "Missing host IP address.\n");
		print_help();
		return -1;
	}
	return optind + 1;
}

int main(const int argc, char **argv)
{
	char *ip_addr = nullptr;
	char *target = nullptr;
	char *rank_str = nullptr;
	char *env_str = nullptr;

	int args_offset = parse_cl_args(argc, argv, &ip_addr, &target, &rank_str, &env_str);
	if (args_offset < 0)
	{
		free_char_arrays(ip_addr, target, rank_str, env_str);
		return EXIT_FAILURE;
	}

	int rank = get_rank(rank_str, env_str);
	if (rank < 0)
	{
		free_char_arrays(ip_addr, target, rank_str, env_str);
		fprintf(stderr, "Could not read rank.\n");
		return EXIT_FAILURE;
	}

	ostringstream tty_gdb_oss;
	ostringstream tty_trgt_oss;
	tty_gdb_oss << "/tmp/ttyGDB_" << setw(4) << setfill('0') << to_string(rank);
	tty_trgt_oss << "/tmp/ttyTRGT_" << setw(4) << setfill('0') << to_string(rank);
	string tty_gdb = tty_gdb_oss.str();
	string tty_trgt = tty_trgt_oss.str();

	int port_gdb = 0x8000 + rank;
	int port_trgt = 0xC000 + rank;

	int pid_socat_gdb = start_socat(tty_gdb, ip_addr, port_gdb);
	int pid_socat_trgt = start_socat(tty_trgt, ip_addr, port_trgt);
	int pid_gdb = start_gdb(argc, argv, args_offset, tty_gdb, tty_trgt, target, pid_socat_gdb, pid_socat_trgt);

	free_char_arrays(ip_addr, target, rank_str, env_str);

	const bool start_success = (pid_gdb > 0) && (pid_socat_gdb > 0) && (pid_socat_trgt > 0);
	int exited = start_success ? 0 : -1;
	while (0 == exited)
	{
		sleep(1);

		exited = 0;

		exited |= waitpid(pid_gdb, nullptr, WNOHANG);
		exited |= waitpid(pid_socat_gdb, nullptr, WNOHANG);
		exited |= waitpid(pid_socat_trgt, nullptr, WNOHANG);
	}

	if (pid_gdb > 0)
	{
		kill(pid_gdb, SIGINT);
	}
	if (pid_socat_gdb > 0)
	{
		kill(pid_socat_gdb, SIGINT);
	}
	if (pid_socat_trgt > 0)
	{
		kill(pid_socat_trgt, SIGINT);
	}

	return EXIT_SUCCESS;
}