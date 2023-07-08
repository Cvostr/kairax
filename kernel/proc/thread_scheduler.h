#ifndef _THREAD_SCHEDULER_H
#define _THREAD_SCHEDULER_H

#include "thread.h"

void init_scheduler();

void scheduler_start();

void scheduler_add_thread(struct thread* thread);

void scheduler_remove_thread(struct thread* thread);

void scheduler_remove_process_threads(struct process* process);

struct thread* scheduler_get_current_thread();

extern void scheduler_yield();

#endif