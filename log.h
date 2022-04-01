#pragma once

#include <stdio.h>

#define LOG_INFO(format, ...)    \
  do {                           \
    printf("[INFO] ");           \
    printf(format, __VA_ARGS__); \
    printf("\n");                \
  } while (0)

#define LOG_ERROR(format, ...)   \
  do {                           \
    printf("[ERROR] ");          \
    printf(format, __VA_ARGS__); \
    printf("\n");                \
  } while (0)

#define LOG_WARNING(format, ...) \
  do {                           \
    printf("[WARNING] ");        \
    printf(format, __VA_ARGS__); \
    printf("\n");                \
  } while (0)
