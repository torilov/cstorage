#pragma once

#include <sys/epoll.h>

#include "epoll.h"

#define kMaxConnectionsNumber 100000
#define kBufferSize ((ssize_t)(1024 * 1024 * sizeof(char)))

typedef void (*callback_function)(struct epoll* epoll, int fd);

typedef enum { kOpen, kClosed } connection_state;

struct connection {
  char* read_buffer;
  ssize_t read_buffer_length;
  ssize_t read_buffer_target_length;

  char* write_buffer;
  ssize_t write_buffer_length;
  ssize_t write_buffer_target_length;

  connection_state state;
  size_t number;

  callback_function callback;
};

struct connection* make_connection(int fd);
struct connection* get_connection(int fd);
void delete_connection(int fd);

int has_ready_read_buffer(struct connection* connection);
int has_ready_write_buffer(struct connection* connection);
