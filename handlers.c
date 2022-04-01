#include "handlers.h"

#include <string.h>

#include "connection.h"
#include "handler_get.h"
#include "handler_set.h"
#include "log.h"
#include "socket.h"

const int kHeaderLength = 4;

const char* kSaveFilesDirname = NULL;

void listener_callback(struct epoll* epoll, int fd) {
  int new_socket_fd = accept_new_socket(fd);
  if (new_socket_fd >= 0) {
    create_connection(epoll, new_socket_fd);
    listen_to_read_events(epoll, new_socket_fd);
  }

  struct connection* connection = get_connection(new_socket_fd);
  LOG_INFO("[connection %ld] new connection", connection->number);
  connection->callback = &read_request_header_callback;
}

void read_request_header_callback(struct epoll* /*epoll*/, int fd) {
  struct connection* connection = get_connection(fd);

  connection->read_buffer_target_length = kHeaderLength;

  async_read_some(fd);

  if (has_ready_read_buffer(connection)) {
    if (!strncmp(connection->read_buffer, "SET\n", 4)) {
      LOG_INFO("[connection %ld] called set", connection->number);
      connection->callback = &handle_set_callback;
      connection->read_buffer_length = 0;
      return;
    }

    if (!strncmp(connection->read_buffer, "GET\n", 4)) {
      LOG_INFO("[connection %ld] called get", connection->number);
      connection->callback = &handle_get_callback;
      connection->read_buffer_length = 0;
      return;
    }

    LOG_ERROR("[connection %ld] incorrect header", connection->number);
    connection->state = kClosed;
  }
}
