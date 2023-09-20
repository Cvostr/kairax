#ifndef _PROCESS_H
#define _PROCESS_H

#include <sys/types.h>
#include "stddef.h"

#define PROTECTION_WRITE_ENABLE    0b1
#define PROTECTION_EXEC_ENABLE     0b10
#define PROTECTION_USER            0b100

int create_thread(void* entry, void* arg, pid_t* pid);

int create_thread_ex(void* entry, void* arg, pid_t* pid, size_t stack_size);

pid_t thread_get_id();

pid_t process_get_id();

void sleep(int time);

void* mmap(void* addr, size_t length, int protection, int flags);

#endif