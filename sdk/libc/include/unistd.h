#ifndef _UNISTD_H
#define _UNISTD_H

#include "sys/types.h"
#include "stddef.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t read(int fd, char* buffer, size_t size);

ssize_t write(int fd, const char* buffer, size_t size);

int close(int fd);

#endif