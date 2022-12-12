#ifndef SERVER_HPP
#define SERVER_HPP

#include "defs.hpp"
#include "window.hpp"
#include "dialog.hpp"

#include <libssh/libssh.h>

int run_cmd(ssh_session &session, UIDialog &dialog);
bool start_clients_srun(UIDialog &dialog);
int start_clients_mpi(UIDialog &dialog);
void process_session(tcp::socket socket, UIWindow &window, const int port);
void start_acceptor(UIWindow &window, const asio::ip::port_type process_port);
void start_servers(UIWindow &window);

#endif /* SERVER_HPP */