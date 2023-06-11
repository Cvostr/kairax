#ifndef _THREAD_SCHEDULER
#define _THREAD_SCHEDULER

#include "thread.h"

void init_scheduler();

void scheduler_start();

void scheduler_add_thread(thread_t* thread);

void scheduler_remove_thread(thread_t* thread);

void scheduler_remove_process_threads(process_t* process);

thread_t* scheduler_get_current_thread();

#endif