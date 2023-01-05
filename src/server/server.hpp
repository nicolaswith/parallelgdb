#ifndef SERVER_HPP
#define SERVER_HPP

#include "defs.hpp"
#include "window.hpp"
#include "startup.hpp"

#include <libssh/libssh.h>

int run_cmd(ssh_session &session, const StartupDialog &dialog);
bool start_clients_ssh(const StartupDialog &dialog);
int start_clients_mpi(const StartupDialog &dialog);
void process_session(tcp::socket socket, UIWindow &window, const int port);
void start_acceptor(UIWindow &window, const asio::ip::port_type port);
void start_servers(UIWindow &window);

#endif /* SERVER_HPP */