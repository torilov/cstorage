#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "panic.h"

const char* usage =
    "cstorage client\n"
    "usage:\n"
    "  cstorage-client hostname port set ./filename\n"
    "  cstorage-client hostname port get file-id-received-on-set\n";

long file_size(const char* filename) {
  FILE* fp = NULL;
  fp = fopen(filename, "r");
  if (fp == NULL) {
    panic_errno();
  }
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  if (fclose(fp) != 0) {
    panic_errno();
  }
  return size;
}

// TODO(torilov): remove unnamed constants.
void set(int socket_fd, const char* filename) {
  write(socket_fd, "SET\n", 4);

  long length = file_size(filename);
  char content_length[10];
  sprintf(content_length, "%08lx\n", length);
  write(socket_fd, content_length, 9);

  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    panic_errno();
  }

  ssize_t buffer_length = 0;
  char buf[1024];
  do {
    buffer_length = read(fd, buf, 1024);
    // TODO(torilov): handle errors.
    write(socket_fd, buf, buffer_length);
  } while (buffer_length > 0);

  if (buffer_length < 0) {
    panic_errno();
  }

  if (close(fd) < 0) {
    panic_errno();
  }

  char file_id[38];
  memset(file_id, '\0', 38);
  buffer_length = read(socket_fd, file_id, 37);
  if (buffer_length != 37) {
    panic("%s", "no response from server");
  }
  printf("%s", file_id);
}

void get(int socket_fd, const char* file_id) {
  if (strlen(file_id) != 36) {
    panic("incorrect file id length; got %ld, but 36 expected",
          strlen(file_id));
  }

  write(socket_fd, "GET\n", 4);
  write(socket_fd, file_id, 36);
  write(socket_fd, "\n", 1);

  ssize_t buffer_length = 0;
  char buf[1024];
  do {
    buffer_length = read(socket_fd, buf, 1024);
    write(1, buf, buffer_length);
  } while (buffer_length > 0);
}

int connect_and_get_fd(const char* host, const char* port) {
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  int err = getaddrinfo(host, port, &hints, &res);
  if (err != 0) {
    panic("%s", gai_strerror(err));
  }
  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    panic_errno();
  }
  if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
    close(fd);
    panic_errno();
  }
  freeaddrinfo(res);
  return fd;
}

int main(int argc, const char* argv[]) {
  setbuf(stdout, NULL);
  if (argc != 5) {
    puts(usage);
    exit(EXIT_FAILURE);
  }

  if (!strncmp(argv[3], "set", 3)) {
    int fd = connect_and_get_fd(argv[1], argv[2]);
    set(fd, argv[4]);
    close(fd);
  } else if (!strncmp(argv[3], "get", 3)) {
    int fd = connect_and_get_fd(argv[1], argv[2]);
    get(fd, argv[4]);
    close(fd);
  } else {
    panic("incorrect parameter %s; get/set expected", argv[3]);
  }

  exit(EXIT_SUCCESS);
}
