#ifndef SERVER_HPP
#define SERVER_HPP

#include "defs.hpp"
#include "window.hpp"
#include "startup.hpp"

#include <libssh/libssh.h>

int run_cmd(ssh_session &session, StartupDialog &dialog);
bool start_clients_srun(StartupDialog &dialog);
int start_clients_mpi(StartupDialog &dialog);
void process_session(tcp::socket socket, UIWindow &window, const int port);
void start_acceptor(UIWindow &window, const asio::ip::port_type process_port);
void start_servers(UIWindow &window);

#endif /* SERVER_HPP */