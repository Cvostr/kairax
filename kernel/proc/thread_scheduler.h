#ifndef _THREAD_SCHEDULER_H
#define _THREAD_SCHEDULER_H

#include "thread.h"

void init_scheduler();

void scheduler_start();

void scheduler_add_thread(struct thread* thread);

void scheduler_remove_thread(struct thread* thread);

void scheduler_remove_process_threads(struct process* process);

struct thread* scheduler_get_next_runnable_thread();

void scheduler_yield(int save_context);

void scheduler_sleep(void* handle, spinlock_t* lock);

int scheduler_wakeup(void* handle);

void scheduler_unblock(struct thread* thread);

#endif