#include "arguments.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "panic.h"

const char* usage =
    "cstorage\n"
    "usage: cstorage port ./folder\n";

struct arguments parse_arguments(int argc, const char* argv[]) {
  if (argc != 3) {
    puts(usage);
    exit(EXIT_FAILURE);
  }

  struct arguments result;

  errno = 0;
  result.port = (int)strtol(argv[1], NULL, 10);
  if (result.port == 0) {
    panic("incorrect port %s", argv[1]);
  }

  result.dirname = argv[2];

  return result;
}
