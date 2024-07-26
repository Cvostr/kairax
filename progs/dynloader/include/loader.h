#ifndef _LOADER_H
#define _LOADER_H

#include "_elf.h"

struct object_data* load_object_data(char* data, int shared);

struct object_data* load_object_data_fd(int fd, int shared);

void process_relocations(struct object_data* obj_data);

int open_shared_object_file(const char* fname);

#endif