#ifndef _THREAD_SCHEDULER
#define _THREAD_SCHEDULER

#include "thread.h"

void init_scheduler();

void start_scheduler();

void add_thread(thread_t* thread);

void remove_thread(thread_t* thread);

#endif