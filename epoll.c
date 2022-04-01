#include "epoll.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "connection.h"
#include "handlers.h"
#include "log.h"
#include "panic.h"
#include "socket.h"

struct epoll* make_epoll() {
  struct epoll* epoll = malloc(sizeof(struct epoll));

  epoll->fd = epoll_create1(0);
  if (epoll->fd < 0) {
    panic_errno();
  }

  epoll->events = malloc(kMaxConnectionsNumber * sizeof(struct epoll_event));
  if (epoll->events == NULL) {
    panic_errno();
  }

  return epoll;
}

void handle_connection(struct epoll* epoll, int fd) {
  struct connection* connection = get_connection(fd);
  assert(connection->callback != NULL);
  connection->callback(epoll, fd);
  if (connection->state == kClosed) {
    delete_connection(fd);
    if (close(fd) < 0) {
      panic_errno();
    }
  }
}

__attribute__((noreturn)) void serve_connections(struct epoll* epoll) {
  for (;;) {
    int number_of_ready_descriptors =
        epoll_wait(epoll->fd, epoll->events, kMaxConnectionsNumber, -1);

    for (int index = 0; index < number_of_ready_descriptors; ++index) {
      handle_connection(epoll, epoll->events[index].data.fd);
    }
  }
}

void add_listener_fd(struct epoll* epoll, int fd) {
  make_socket_non_blocking(fd);

  struct connection* connection = make_connection(fd);

  struct epoll_event event = {0};
  event.data.fd = fd;
  event.events = EPOLLIN;

  if (epoll_ctl(epoll->fd, EPOLL_CTL_ADD, fd, &event) < 0) {
    panic_errno();
  }

  connection->callback = &listener_callback;
}

void create_connection(struct epoll* /*epoll*/, int fd) {
  if (fd > kMaxConnectionsNumber) {
    close(fd);
    LOG_ERROR("too much connections: %d", fd);
    return;
  }
  make_socket_non_blocking(fd);
  make_connection(fd);
}

void listen_to_read_events(struct epoll* epoll, int fd) {
  struct epoll_event event = {0};
  event.data.fd = fd;
  event.events |= EPOLLIN;

  if (epoll_ctl(epoll->fd, EPOLL_CTL_ADD, fd, &event) < 0) {
    panic_errno();
  }
}

void mute(struct epoll* epoll, int fd) {
  if (epoll_ctl(epoll->fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
    panic_errno();
  }
}

void listen_to_write_events(struct epoll* epoll, int fd) {
  struct epoll_event event = {0};
  event.data.fd = fd;
  event.events |= EPOLLOUT;

  if (epoll_ctl(epoll->fd, EPOLL_CTL_ADD, fd, &event) < 0) {
    panic_errno();
  }
}
