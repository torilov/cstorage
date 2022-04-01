#pragma once

#include <errno.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define panic(format, ...)                          \
  do {                                              \
    printf("panic at %s:%d\n", __FILE__, __LINE__); \
    printf(format, __VA_ARGS__);                    \
    printf("\n");                                   \
    __builtin_trap();                               \
  } while (0)

#define panic_errno()                               \
  do {                                              \
    printf("panic at %s:%d\n", __FILE__, __LINE__); \
    perror(strerror(errno));                        \
    printf("\n");                                   \
    __builtin_trap();                               \
  } while (0)
