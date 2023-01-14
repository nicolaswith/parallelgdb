# Parallel GDB
This program is for debugging parallel programs either on the local machine or on a remote cluster using SSH. It provides support to launch the processes via mpirun and srun.

# Structure
This debugger consists of two parts: 
1. The GUI server running on the host machine
2. The clients running on the host machine or the remote cluster

# Building
To build these two separate executables, you can use the following command from the top level directory.

	mkdir bin
	make

This will generate a `server` and a `client` executable in the `bin` directory.

If you only want a specific executable you can run 

	make server

or 

	make client

to get the corresponding executable.

Use

	make clean

to delete all generated files.

# Dependencies
## Compile-Time
The following dependencies must be available for building:
- libgtkmm-3.0
- libgtksourceviewmm-3.0
- libssh

On Debian based systems, you can use the following command to install them:

	apt install libgtkmm-3.0-dev libgtksourceviewmm-3.0-dev libssh-dev

## Run-Time
For the debugger to work, the following programs need to be installed:
- mpi / slurm: To start the client instances.
- gdb: To debug the target program.
- socat: To handle the I/O of the gdb instance and the target instance.

Furthermore, when debugging on a remote cluster, the `client` executable needs to be copied to or build on this machine.

# Using the debugger
The debugging server is started by using the command:

	./bin/server

In the start dialog you need to set all the corresponding paths and parameters. This configuration can be exported and imported at the next start.

The server will start the specified number of clients, each of which will start the gdb instance, running the target program, and two socat instances, handling the I/O of gdb and the target.

If SSH is enabled, the server logs on to the remote cluster and starts the clients there.
