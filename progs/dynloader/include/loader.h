#ifndef _LOADER_H
#define _LOADER_H

#include "include/elf.h"

struct object_data* load_object_data(char* data, int shared);

struct object_data* load_object_data_fd(int fd, int shared);

int open_shared_object_file(const char* fname);

#endif