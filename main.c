#include <stdio.h>
#include <stdlib.h>

#include "arguments.h"
#include "epoll.h"
#include "handlers.h"
#include "log.h"
#include "socket.h"

int main(int argc, const char* argv[]) {
  setbuf(stdout, NULL);
  srand(42);  // TODO(torilov): initialize as command line argument.

  struct arguments arguments = parse_arguments(argc, argv);
  kSaveFilesDirname = arguments.dirname;

  struct epoll* epoll = make_epoll();
  int listener_socket_fd = make_listener_socket(arguments.port);
  add_listener_fd(epoll, listener_socket_fd);

  LOG_INFO("serving on port %d with folder %s", arguments.port,
           arguments.dirname);

  serve_connections(epoll);  // TODO(torilov): add graceful shutdown.
}
