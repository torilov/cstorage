#pragma once

int make_listener_socket(int port);
void make_socket_non_blocking(int socket_fd);
int accept_new_socket(int listener_fd);
void async_read_some(int fd);
void async_write_some(int fd);
