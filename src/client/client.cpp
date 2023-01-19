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

#include <cstdlib>
#include <iostream>
#include <string>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <array>
#include <sys/wait.h>

using namespace std;

void wait_for_socat(const int pid_socat_gdb, const int pid_socat_trgt)
{
	for (;;)
	{
		FILE *cmd = popen("pidof -x socat", "r");
		char result[1024] = {0};
		while (fgets(result, sizeof(result), cmd) != NULL)
		{
			int pid;
			bool found_gdb = false;
			bool found_trgt = false;
			stringstream stream(result);
			while (stream >> pid)
			{
				if (pid == pid_socat_gdb)
					found_gdb = true;
				if (pid == pid_socat_trgt)
					found_trgt = true;
			}
			if (found_gdb && found_trgt)
				return;
		}
		pclose(cmd);
		usleep(100);
	}
}

int start_gdb(const string &tty_gdb, const string &tty_trgt, const char *const gdb_path, const char *const target, const int pid_socat_gdb, const int pid_socat_trgt)
{
	wait_for_socat(pid_socat_gdb, pid_socat_trgt);
	usleep(500000);

	int pid = fork();
	if (0 == pid)
	{
		int tty_fd = open(tty_gdb.c_str(), O_RDWR);

		dup2(tty_fd, STDIN_FILENO);
		dup2(tty_fd, STDOUT_FILENO);
		dup2(tty_fd, STDERR_FILENO);

		string tty = "--tty=" + tty_trgt;

		char *argv[] = {
			(char *)gdb_path,
			(char *)"-q",
			(char *)"-i",
			(char *)"mi3",
			(char *)"-ex=set auto-load safe-path /",
			(char *)"-ex=b main",
			(char *)tty.c_str(),
			(char *)target,
			(char *)nullptr};
		execvp(argv[0], argv);
		_exit(127);
	}

	return pid;
}

int start_socat(const string &tty_name, const char *const socat_path, const char *const ip_addr, const int port)
{
	int pid = fork();
	if (0 == pid)
	{
		string tty = "pty,echo=0,link=" + tty_name;
		string tcp = "TCP:" + string(ip_addr) + ":" + to_string(port);

		char *argv[] = {
			(char *)socat_path,
			(char *)tty.c_str(),
			(char *)tcp.c_str(),
			(char *)nullptr};
		execvp(argv[0], argv);
		_exit(127);
	}
	return pid;
}

int get_rank(const char *const rank_str)
{
	const char *rank = nullptr;
	if (rank_str)
	{
		rank = rank_str;
	}
	else
	{
		std::array<const char *const, 3> env_vars = {
			"PMI_RANK",
			"OMPI_COMM_WORLD_RANK",
			"PGDB_RANK"};
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
	catch (const std::exception &e)
	{
		return -1;
	}
}

void print_help()
{
	fprintf(
		stderr,
		"Usage: ./client [OPTIONS] </path/to/target>\n"
		"Options:\n"
		"  -g <path>\t path to gdb\n"
		"  -s <path>\t path to socat\n"
		"  -i <addr>\t host IP address\n"
		"  -r <rank>\t rank of process\n"
		"  -h\t\t print this help\n");
}

bool parse_cl_args(const int argc, char **argv, char **ip_addr, char **gdb_path, char **socat_path, char **target, char **rank_str)
{
	char c;
	opterr = 0;
	while ((c = getopt(argc, argv, "hg:s:i:r:")) != -1)
	{
		switch (c)
		{
		case 'g': // gdb
			free(*gdb_path);
			*gdb_path = strdup(optarg);
			break;
		case 's': // socat
			free(*socat_path);
			*socat_path = strdup(optarg);
			break;
		case 'i': // ip
			free(*ip_addr);
			*ip_addr = strdup(optarg);
			break;
		case 'r': // rank
			free(*rank_str);
			*rank_str = strdup(optarg);
			break;
		case 'h': // help
			print_help();
			free(*target);
			free(*socat_path);
			free(*gdb_path);
			free(*ip_addr);
			free(*rank_str);
			exit(EXIT_SUCCESS);
			break;
		case '?':
			if ('i' == optopt)
			{
				fprintf(stderr, "Option -%c requires the host IP address.\n", optopt);
			}
			else if ('s' == optopt)
			{
				fprintf(stderr, "Option -%c requires the path to socat.\n", optopt);
			}
			else if ('g' == optopt)
			{
				fprintf(stderr, "Option -%c requires the path to gdb.\n", optopt);
			}
			else if ('r' == optopt)
			{
				fprintf(stderr, "Option -%c requires the process rank.\n", optopt);
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

	if (argc - optind == 1)
	{
		*target = strdup(argv[optind]);
	}
	else
	{
		if (argc == optind)
		{
			fprintf(stderr, "No target specified.\n");
		}
		else
		{
			fprintf(stderr, "Too many arguments.\n");
		}
		print_help();
		return false;
	}

	return true;
}

int main(const int argc, char **argv)
{
	char *ip_addr = nullptr;
	char *gdb_path = nullptr;
	char *socat_path = nullptr;
	char *target = nullptr;
	char *rank_str = nullptr;
	if (!parse_cl_args(argc, argv, &ip_addr, &gdb_path, &socat_path, &target, &rank_str))
	{
		free(target);
		free(socat_path);
		free(gdb_path);
		free(ip_addr);
		free(rank_str);
		return EXIT_FAILURE;
	}

	int pid = getpid();
	string tty_gdb = "/tmp/ttyGDB_" + to_string(pid);
	string tty_trgt = "/tmp/ttyTRGT_" + to_string(pid);

	int rank = get_rank(rank_str);
	if (rank < 0)
	{
		free(target);
		free(socat_path);
		free(gdb_path);
		free(ip_addr);
		free(rank_str);
		fprintf(stderr, "Could not read rank.\n");
		return EXIT_FAILURE;
	}

	if (target == nullptr ||
		socat_path == nullptr ||
		gdb_path == nullptr ||
		ip_addr == nullptr)
	{
		fprintf(stderr, "Missing configuration. [paths or/and IP address]\n");
	}

	int port_gdb = 0x8000 + rank;
	int port_trgt = 0xC000 + rank;

	int pid_socat_gdb = start_socat(tty_gdb, socat_path, ip_addr, port_gdb);
	int pid_socat_trgt = start_socat(tty_trgt, socat_path, ip_addr, port_trgt);
	int pid_gdb = start_gdb(tty_gdb, tty_trgt, gdb_path, target, pid_socat_gdb, pid_socat_trgt);

	free(target);
	free(socat_path);
	free(gdb_path);
	free(ip_addr);
	free(rank_str);

	int status;
	while (true)
	{
		int exited = 0;
		exited |= waitpid(pid_gdb, &status, WNOHANG);
		exited |= waitpid(pid_socat_gdb, &status, WNOHANG);
		exited |= waitpid(pid_socat_trgt, &status, WNOHANG);

		if (exited)
			break;

		sleep(1);
	}

	kill(pid_gdb, SIGINT);
	kill(pid_socat_gdb, SIGINT);
	kill(pid_socat_trgt, SIGINT);

	return EXIT_SUCCESS;
}