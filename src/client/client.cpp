#include <cstdlib>
#include <iostream>
#include <string>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
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

int start_gdb(string tty_gdb, string tty_trgt, const char *const gdb_path, const char *const target, const int pid_socat_gdb, const int pid_socat_trgt)
{
	wait_for_socat(pid_socat_gdb, pid_socat_trgt);
	usleep(500000); // 500ms

	int pid = fork();
	if (0 == pid)
	{
		int tty_fd = open(tty_gdb.c_str(), O_RDWR);

		// connect to virtual io
		dup2(tty_fd, STDIN_FILENO);
		dup2(tty_fd, STDOUT_FILENO);
		dup2(tty_fd, STDERR_FILENO);

		string tty = "--tty=" + tty_trgt;

		char *argv[] = {
			(char *)gdb_path,
			(char *)"--interpreter=mi",
			// evil hack for now... TODO
			(char *)"-ex=set auto-load safe-path /",
			(char *)tty.c_str(),
			(char *)target,
			(char *)nullptr};
		execvp(argv[0], argv);
		_exit(127);
	}

	return pid;
}

int start_socat(string tty_name, const char *const socat_path, const char *const ip_addr, const int port)
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

int get_process_rank(const char *const env_name)
{
	const char *process_rank = getenv(env_name);

	if (!process_rank)
		return -1;
	try
	{
		return stoi(process_rank);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error reading rank env: " << e.what() << '\n';
		return -1;
	}
}

void print_help()
{
	fprintf(
		stderr,
		"Usage: ./client [OPTIONS] </path/to/target>\n"
		"Options:\n"
		"  -r\t\t use srun\n"
		"  -g <path>\t path to gdb\n"
		"  -s <path>\t path to socat\n"
		"  -i <addr>\t host IP address\n"
		"  -h\t\t print this help\n");
}

int main(const int argc, char **argv)
{
	char c;
	bool use_srun = false;
	char *ip_addr = nullptr;
	char *gdb_path = nullptr;
	char *socat_path = nullptr;
	opterr = 0;

	while ((c = getopt(argc, argv, "hrg:s:i:")) != -1)
	{
		switch (c)
		{
		case 'g': // gdb
			free(gdb_path);
			gdb_path = strdup(optarg);
			break;
		case 's': // socat
			free(socat_path);
			socat_path = strdup(optarg);
			break;
		case 'i': // ip
			free(ip_addr);
			ip_addr = strdup(optarg);
			break;
		case 'r': // remote
			use_srun = true;
			break;
		case 'h': // help
			print_help();
			return EXIT_SUCCESS;
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
			return EXIT_FAILURE;
		}
	}

	char *target;
	if (argc - optind == 1)
	{
		target = strdup(argv[optind]);
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
		free(socat_path);
		free(gdb_path);
		free(ip_addr);
		print_help();
		return EXIT_FAILURE;
	}

	char *env_name;
	if (use_srun)
	{
		env_name = strdup("PMI_RANK");
	}
	else
	{
		env_name = strdup("OMPI_COMM_WORLD_RANK");
	}

	int pid = getpid();
	string tty_gdb = "/tmp/ttyGDB_" + to_string(pid);
	string tty_trgt = "/tmp/ttyTRGT_" + to_string(pid);

	int process_rank = get_process_rank(env_name);
	if (process_rank < 0)
	{
		free(target);
		free(env_name);
		free(socat_path);
		free(gdb_path);
		free(ip_addr);
		fprintf(stderr, "Could not read process_rank.\n");
		return EXIT_FAILURE;
	}

	int port_gdb = 0x8000 + process_rank;
	int port_trgt = 0xC000 + process_rank;

	int pid_socat_gdb = start_socat(tty_gdb, socat_path, ip_addr, port_gdb);
	int pid_socat_trgt = start_socat(tty_trgt, socat_path, ip_addr, port_trgt);
	int pid_gdb = start_gdb(tty_gdb, tty_trgt, gdb_path, target, pid_socat_gdb, pid_socat_trgt);

	free(target);
	free(env_name);
	free(socat_path);
	free(gdb_path);
	free(ip_addr);

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