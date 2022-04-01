#pragma once

#include "epoll.h"

#define kFileNameLength 36

extern const char* kSaveFilesDirname;

void listener_callback(struct epoll* epoll, int fd);
void read_request_header_callback(struct epoll* epoll, int fd);
