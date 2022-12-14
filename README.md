# Parallel GDB
This program is for debugging parallel programs either on a remote cluster using srun or on the local machine with mpirun. 

# Structure
This debugger consists of two parts: 
1. The GUI server running on the host machine
2. The GDB client(s) running on the host machine or the remote cluster

# Building
To build these two separate executables you can use the following command from the top level directory.

	mkdir bin
	make

This will generate a `server` and a `client` executable in the `bin` folder.

If you only want a specific executable you can run 

	make server

or 

	make client

to get the corresponding executable.

Use 

	make clean

to delete all generated files.

If you want to edit the glade file you might need to copy the file `./ui/gtksourceview.xml` to `/usr/share/glade/catalogs/gtksourceview.xml` as installing the `libgtksourceview-3.0-dev` package did not create this file (on my machine...).

I created this file by building the gtksourceview package [gtksourceview-3.24.11.tar.xz](https://download.gnome.org/sources/gtksourceview/3.24/gtksourceview-3.24.11.tar.xz) from source.

# Dependencies
The following dependencies must be available for building:
- libgtkmm-3.0
- libgtksourceviewmm-3.0
- libssh

On Debian based systems you can use the following command to install them:

	apt install libgtkmm-3.0-dev libgtksourceviewmm-3.0-dev libssh-dev

These libraries must be installed on the host:
- make
- g++
- mpi
- socat

And these on the remote machine:
- socat
- slurm

# Using the debugger
## Local debugging
To run the debugger on the host machine using mpirun, start the server by running 

	./bin/server

## Remote debugging
To use the debugger on a remote cluster the client executable from the `bin` directory must be copied onto the remote machine. The debugging GUI server can then be startet with

	./bin/server

After setting all the according paths and parameters in the opening dialog, the server will log in on the remote machine via SSH and start the clients automatically.
