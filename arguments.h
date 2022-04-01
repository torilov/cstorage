#pragma once

struct arguments {
  int port;
  const char* dirname;
};

struct arguments parse_arguments(int argc, const char* argv[]);
