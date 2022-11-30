#ifndef SERVER_HPP
#define SERVER_HPP

#include "defs.hpp"
#include "window.hpp"
#include "dialog.hpp"

#include <libssh/libssh.h>

int run_cmd(ssh_session &a_session, UIDialog &a_dialog);
bool start_clients_srun(UIDialog &a_dialog);
int start_clients_mpi(UIDialog &a_dialog);
void process_session(tcp::socket a_sock, UIWindow &a_window, const int a_port);
void start_acceptor(UIWindow &a_window, const asio::ip::port_type a_process_port);
void start_servers(UIWindow &a_window);

#endif /* SERVER_HPP */