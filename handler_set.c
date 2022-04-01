#include "handler_set.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "connection.h"
#include "handlers.h"
#include "log.h"
#include "panic.h"
#include "socket.h"

void handle_set_read_request_body_length(struct epoll* epoll, int fd);
void handle_set_read_request_body_callback(struct epoll* epoll, int fd);
void handle_set_write_to_file(const char* data, ssize_t data_length,
                              char* output_filename);
void handle_set_write_request_body_callback(struct epoll* epoll, int fd);

void handle_set_callback(struct epoll* epoll, int fd) {
  handle_set_read_request_body_length(epoll, fd);
}

const int kBodyLengthStringRepresentationLength = 9;

void handle_set_read_request_body_length(struct epoll* /*epoll*/, int fd) {
  struct connection* connection = get_connection(fd);
  connection->read_buffer_target_length = kBodyLengthStringRepresentationLength;

  async_read_some(fd);

  if (has_ready_read_buffer(connection)) {
    connection->read_buffer_target_length =
        strtoll(connection->read_buffer, NULL, 16);

    if (connection->read_buffer_target_length == 0) {
      LOG_ERROR("[connection %ld] incorrect file length", connection->number);
      connection->state = kClosed;
      return;
    }

    if (connection->read_buffer_target_length > kBufferSize) {
      LOG_ERROR("[connection %ld] file is too big", connection->number);
      connection->state = kClosed;
      return;
    }

    connection->read_buffer_length = 0;
    connection->callback = &handle_set_read_request_body_callback;
  }
}

void handle_set_read_request_body_callback(struct epoll* epoll, int fd) {
  struct connection* connection = get_connection(fd);

  async_read_some(fd);

  if (has_ready_read_buffer(connection)) {
    LOG_INFO("[connection %ld] save data to disk", connection->number);

    // Save filename directly to connection->write_buffer.
    handle_set_write_to_file(connection->read_buffer,
                             connection->read_buffer_length,
                             connection->write_buffer);
    connection->write_buffer_target_length = kFileNameLength;
    connection->write_buffer[connection->write_buffer_target_length++] = '\n';
    LOG_INFO("[connection %ld] saved", connection->number);

    mute(epoll, fd);
    listen_to_write_events(epoll, fd);
    connection->callback = &handle_set_write_request_body_callback;
  }
}

void fill_with_random_symbols(char* output, int length) {
  static const char* alphabet =
      "0123456789"
      "abcdefghijklmnopqrstuvwxyz";
  for (int index = 0; index < length; ++index) {
    output[index] = alphabet[rand() % strlen(alphabet)];
  }
}

// Blocking write to file.
void handle_set_write_to_file(const char* data, ssize_t data_length,
                              char* output_filename) {
  // TODO(torilov): consider making this operation non-blocking.
  // TODO(torilov): split to several functions.

  // Choose unused filename.
  size_t full_name_length = strlen("./") + strlen(kSaveFilesDirname) +
                            strlen("/") + kFileNameLength + 1;

  char full_name[full_name_length];

  for (;;) {
    fill_with_random_symbols(output_filename, kFileNameLength);

    snprintf(full_name, full_name_length, "./%s/%s", kSaveFilesDirname,
             output_filename);

    if (access(full_name, F_OK)) {
      break;
    }
    LOG_WARNING("filename %s is already taken", full_name);
  }

  // Write data to file.
  int output_fd = open(full_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (output_fd < 0) {
    panic_errno();
  }

  ssize_t write_index = 0;
  while (write_index < data_length) {
    write_index +=
        write(output_fd, data + write_index, data_length - write_index);
  }

  // Ensure all data is saved on disk.
  if (fsync(output_fd)) {
    panic_errno();
  }
  if (close(output_fd)) {
    panic_errno();
  }

  int directory_fd = open(kSaveFilesDirname, O_RDONLY);
  if (directory_fd < 0) {
    panic_errno();
  }
  if (fsync(directory_fd)) {
    panic_errno();
  }
  if (close(directory_fd)) {
    panic_errno();
  }

  LOG_INFO("saved data to file %s", full_name);
}

void handle_set_write_request_body_callback(struct epoll* epoll, int fd) {
  struct connection* connection = get_connection(fd);

  async_write_some(fd);

  if (has_ready_write_buffer(connection)) {
    mute(epoll, fd);
    LOG_INFO("[connection %ld] response is sent, closing", connection->number);
    connection->state = kClosed;
  }
}
