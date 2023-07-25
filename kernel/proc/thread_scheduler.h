#ifndef _THREAD_SCHEDULER_H
#define _THREAD_SCHEDULER_H

#include "thread.h"

void init_scheduler();

void scheduler_start();

void scheduler_add_thread(struct thread* thread);

void scheduler_remove_thread(struct thread* thread);

void scheduler_remove_process_threads(struct process* process);

void scheduler_yield();

void scheduler_from_killed();

#endif