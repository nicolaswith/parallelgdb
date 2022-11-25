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
		std::cerr << e.what() << '\n';
		return -1;
	}
}

void print_help()
{
	fprintf(
		stderr,
		"Usage: ./client [OPTIONS] </path/to/target>\n"
		"Options:\n"
		"  -m\t\t use mpirun (default)\n"
		"  -s <ip.addr>\t use srun\n"
		"  -h\t\t print this help\n");
}

int main(const int argc, char **argv)
{
	char c;
	bool use_srun = false;
	char *ip_addr = strdup("localhost");
	opterr = 0;

	while ((c = getopt(argc, argv, "mhs:")) != -1)
	{
		switch (c)
		{
		case 'm':
			use_srun = false;
			break;
		case 's':
			use_srun = true;
			free(ip_addr);
			ip_addr = strdup(optarg);
			break;
		case 'h':
			print_help();
			return EXIT_SUCCESS;
		case '?':
			if ('s' == optopt)
			{
				fprintf(stderr, "Option -s requires the host ip address.\n");
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
	if (argc == optind)
	{
		fprintf(stderr, "No target specified.\n");
		print_help();
		free(ip_addr);
		return EXIT_FAILURE;
	}
	else
	{
		target = strdup(argv[optind]);
	}

	char *env_name;
	char *socat_path;
	char *gdb_path;
	if (use_srun)
	{
		env_name = strdup("PMI_RANK");
		socat_path = strdup("/home/urz/with/spack/opt/spack/linux-centos8-x86_64_v3/gcc-8.5.0/socat-1.7.4.4-glonybxmsca7crv7c4vi2s4fnrmg7ebp/bin/socat");
		gdb_path   = strdup("/home/urz/with/spack/opt/spack/linux-centos8-x86_64_v3/gcc-8.5.0/gdb-12.1-yebecfkufo6ycigccpxnskmicviekwb7/bin/gdb");
	}
	else
	{
		env_name = strdup("OMPI_COMM_WORLD_RANK");
		socat_path = strdup("/usr/bin/socat");
		gdb_path   = strdup("/usr/bin/gdb");
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

	int port_gdb  = 0x8000 + process_rank;
	int port_trgt = 0xC000 + process_rank;

	int pid_socat_gdb  = start_socat(tty_gdb,  socat_path, ip_addr, port_gdb);
	int pid_socat_trgt = start_socat(tty_trgt, socat_path, ip_addr, port_trgt);
	int pid_gdb        = start_gdb(tty_gdb, tty_trgt, gdb_path, target, pid_socat_gdb, pid_socat_trgt);

	free(target);
	free(env_name);
	free(socat_path);
	free(gdb_path);
	free(ip_addr);

	int status;

	while (true)
	{
		int exited = 0;
		exited |= waitpid(pid_gdb,        &status, WNOHANG);
		exited |= waitpid(pid_socat_gdb,  &status, WNOHANG);
		exited |= waitpid(pid_socat_trgt, &status, WNOHANG);

		if (exited)
			break;

		sleep(1);
	}

	kill(pid_gdb,        SIGINT);
	kill(pid_socat_gdb,  SIGINT);
	kill(pid_socat_trgt, SIGINT);

	return EXIT_SUCCESS;
}