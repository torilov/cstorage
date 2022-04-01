#include "connection.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "panic.h"

static size_t connection_number = 0;

// Array of active connections. key: file descriptor, value: connection.
struct connection connections[kMaxConnectionsNumber];

struct connection* make_connection(int fd) {
  struct connection* connection = &connections[fd];

  assert(connection->read_buffer == NULL);
  connection->read_buffer = malloc(kBufferSize);
  if (connection->read_buffer == NULL) {
    panic_errno();
  }
  connection->read_buffer_length = 0;
  connection->read_buffer_target_length = 0;

  assert(connection->write_buffer == NULL);
  connection->write_buffer = malloc(kBufferSize);
  if (connection->write_buffer == NULL) {
    panic_errno();
  }
  connection->write_buffer_length = 0;
  connection->write_buffer_target_length = 0;

  connection->callback = NULL;

  connection->state = kOpen;
  connection->number = connection_number++;

  return connection;
}

struct connection* get_connection(int fd) {
  return &connections[fd];
}

void delete_connection(int fd) {
  struct connection* connection = &connections[fd];

  free(connection->read_buffer);
  free(connection->write_buffer);
  memset(connection, 0, sizeof(struct connection));
}

int has_ready_read_buffer(struct connection* connection) {
  return (connection->state != kClosed) &&
         (connection->read_buffer_length ==
          connection->read_buffer_target_length);
}

int has_ready_write_buffer(struct connection* connection) {
  return (connection->state != kClosed) &&
         (connection->write_buffer_length ==
          connection->write_buffer_target_length);
}
