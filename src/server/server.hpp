#include "defs.hpp"

#ifndef _SERVER_HPP
#define _SERVER_HPP

#include <libssh/libssh.h>

#include "window.hpp"
#include "dialog.hpp"

int run_cmd(ssh_session &session, UIDialog &dialog);
bool start_clients_srun(UIDialog &dialog);
int start_clients_mpi(const int a_num_processes, const char *const a_target);
void process_session(tcp::socket a_sock, UIWindow &a_window, const int a_port);
void start_acceptor(UIWindow &a_window, const asio::ip::port_type a_process_port);
void start_servers(UIWindow &a_window);

#endif /* _SERVER_HPP */