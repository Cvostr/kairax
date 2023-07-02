#ifndef _PROCESS_H
#define _PROCESS_H

#include "types.h"
#include "stddef.h"

int create_thread(void* entry, void* arg, pid_t* pid);

int create_thread_ex(void* entry, void* arg, pid_t* pid, size_t stack_size);

pid_t process_get_id();

void sleep(int time);

#endif