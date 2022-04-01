#include "socket.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "connection.h"
#include "log.h"
#include "panic.h"

static const int kBacklog = 128;

int make_listener_socket(int port) {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    panic_errno();
  }

  int opt = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    panic_errno();
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  if (bind(socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    panic_errno();
  }

  if (listen(socket_fd, kBacklog) < 0) {
    panic_errno();
  }

  return socket_fd;
}

void make_socket_non_blocking(int socket_fd) {
  int flags = fcntl(socket_fd, F_GETFL, 0);
  if (flags == -1) {
    panic_errno();
  }

  if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    panic_errno();
  }
}

int accept_new_socket(int listener_fd) {
  struct sockaddr peer_address;
  socklen_t peer_address_length = sizeof(peer_address);

  int new_socket_fd = accept(listener_fd, (struct sockaddr*)&peer_address,
                             &peer_address_length);

  if (new_socket_fd < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      panic_errno();
    }
  }

  return new_socket_fd;
}

void async_read_some(int fd) {
  struct connection* connection = get_connection(fd);

  ssize_t read_target_length =
      connection->read_buffer_target_length - connection->read_buffer_length;

  ssize_t actual_length =
      recv(fd, connection->read_buffer + connection->read_buffer_length,
           read_target_length * sizeof(char), 0);

  if (actual_length == 0) {
    LOG_WARNING("[connection %ld] disconnected", connection->number);
    connection->state = kClosed;
  } else if (actual_length < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      panic_errno();
    }
    return;
  }

  connection->read_buffer_length += actual_length;
}

void async_write_some(int fd) {
  struct connection* connection = get_connection(fd);

  ssize_t write_target_length =
      connection->write_buffer_target_length - connection->write_buffer_length;

  ssize_t actual_length =
      send(fd, connection->write_buffer + connection->write_buffer_length,
           write_target_length * sizeof(char), 0);

  if (actual_length == 0) {
    LOG_WARNING("[connection %ld] disconnected", connection->number);
    connection->state = kClosed;
  } else if (actual_length < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      panic_errno();
    }
    return;
  }

  connection->write_buffer_length += actual_length;
}
