#pragma once

#include <sys/epoll.h>

struct epoll {
  int fd;
  struct epoll_event* events;
};

struct epoll* make_epoll();
__attribute__((noreturn)) void serve_connections(struct epoll* epoll);
void add_listener_fd(struct epoll* epoll, int fd);
void create_connection(struct epoll* epoll, int fd);
void listen_to_read_events(struct epoll* epoll, int fd);
void mute(struct epoll* epoll, int fd);
void listen_to_write_events(struct epoll* epoll, int fd);
