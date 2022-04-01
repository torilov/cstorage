#include "handler_get.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "handlers.h"
#include "log.h"
#include "panic.h"
#include "socket.h"

void handle_get_read_filename(struct epoll* epoll, int fd);
void handle_get_read_file(struct epoll* epoll, int fd);
void handle_get_send_file_data_callback(struct epoll* epoll, int fd);

void handle_get_callback(struct epoll* epoll, int fd) {
  handle_get_read_filename(epoll, fd);
}

void handle_get_read_filename(struct epoll* epoll, int fd) {
  struct connection* connection = get_connection(fd);
  connection->read_buffer_target_length = kFileNameLength;

  async_read_some(fd);

  if (has_ready_read_buffer(connection)) {
    connection->read_buffer[connection->read_buffer_target_length] = '\0';

    LOG_INFO("[connection %ld] read file %s", connection->number,
             connection->read_buffer);

    handle_get_read_file(epoll, fd);
    connection->write_buffer_length = 0;
    connection->callback = &handle_get_send_file_data_callback;

    mute(epoll, fd);
    listen_to_write_events(epoll, fd);
  }
}

// Blocking read from file.
void handle_get_read_file(struct epoll* /*epoll*/, int fd) {
  struct connection* connection = get_connection(fd);

  size_t full_name_length = strlen("./") + strlen(kSaveFilesDirname) +
                            strlen("/") + kFileNameLength + 1;

  char full_name[full_name_length];

  snprintf(full_name, full_name_length, "./%s/%s", kSaveFilesDirname,
           connection->read_buffer);

  if (access(full_name, F_OK) < 0) {
    LOG_ERROR("[connection %ld] file %s not exists", connection->number,
              connection->read_buffer);
    connection->state = kClosed;
    return;
  }

  int input_fd = open(full_name, O_RDONLY);
  if (input_fd < 0) {
    panic_errno();
  }

  ssize_t index = 0;
  while (index < kBufferSize) {
    ssize_t length = kBufferSize - index;
    ssize_t read_characters =
        read(input_fd, connection->write_buffer + index, length);

    if (read_characters < 0) {
      panic_errno();
    }

    if (read_characters == 0) {
      break;
    }

    index += read_characters;
  }

  if (close(input_fd)) {
    panic_errno();
  }

  connection->write_buffer_target_length = index;
}

void handle_get_send_file_data_callback(struct epoll* epoll, int fd) {
  struct connection* connection = get_connection(fd);

  async_write_some(fd);

  if (has_ready_write_buffer(connection)) {
    mute(epoll, fd);
    LOG_INFO("[connection %ld] response is sent, closing", connection->number);
    connection->state = kClosed;
  }
}
