#ifndef _PROCESS_H
#define _PROCESS_H

#include <sys/types.h>
#include "stddef.h"

pid_t create_thread(void* entry, void* arg);

pid_t create_thread_ex(void* entry, void* arg, size_t stack_size);

pid_t thread_get_id();

pid_t process_get_id();

void sleep(int time);

#endif