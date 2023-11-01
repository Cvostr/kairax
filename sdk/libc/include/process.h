#ifndef _PROCESS_H
#define _PROCESS_H

#include <sys/types.h>
#include "stddef.h"

pid_t create_thread(void* entry, void* arg);

pid_t create_thread_ex(void* entry, void* arg, size_t stack_size);

void thread_exit(int code);

struct process_create_info {
    char*   current_directory;
    int  num_args;
    char**  args;

    int _stdin;
    int _stdout;
    int _stderr;
};

pid_t create_process(int dirfd, const char* filepath, struct process_create_info* info);

#endif