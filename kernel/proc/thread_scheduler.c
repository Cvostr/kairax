#include "thread_scheduler.h"
#include "cpu/cpu_local_x64.h"

#define MAX_THREADS 3000

struct thread* sched_threads[MAX_THREADS];
pid_t current_thread = 0;
pid_t max_tid = 0;
spinlock_t threads_lock = 0;

void init_scheduler()
{
   
}

void scheduler_add_thread(struct thread* thread)
{
    acquire_spinlock(&threads_lock);

    for (pid_t tid = 0; tid < MAX_THREADS; tid ++) {
        if (sched_threads[tid] == NULL) {
            sched_threads[tid] = thread;
            if (max_tid < tid) {
                max_tid = tid;
            }

            break;
        }
    }
    
    release_spinlock(&threads_lock);
}

void scheduler_remove_thread(struct thread* thread)
{
    acquire_spinlock(&threads_lock);
    
    for (uint64 i = 0; i < MAX_THREADS; i ++) {
        if (sched_threads[i] == thread) {
            sched_threads[i] = NULL;
        }
    }

    release_spinlock(&threads_lock);
}

void scheduler_remove_process_threads(struct process* process)
{
    // TODO: Ожидание выхода всех потоков из системных вызовов
    for (size_t i = 0; i < list_size(process->threads); i ++) {
        struct thread* thread = list_get(process->threads, i);
        scheduler_remove_thread(thread);
    }
}

void scheduler_sleep(void* handle, spinlock_t* lock)
{
    struct thread* thr = cpu_get_current_thread();

    if (lock != NULL)
        release_spinlock(lock);

    // Изменяем состояние - блокируемся
    thr->wait_handle = handle;
    thr->state = STATE_INTERRUPTIBLE_SLEEP;

    // Передача управления другому процессу
    scheduler_yield(TRUE);

    if (lock != NULL)
        // Блокируем спинлок
        acquire_spinlock(lock);
}

int scheduler_wakeup(void* handle, int max)
{
    int i = 0;
    acquire_spinlock(&threads_lock);

    for (pid_t tid = 0; tid <= max_tid; tid ++) {

        struct thread* thread = sched_threads[tid];

        if (thread != NULL) {
            if (thread->state == STATE_INTERRUPTIBLE_SLEEP && thread->wait_handle == handle && i < max) {
                scheduler_unblock(thread);
                i ++;
            }
        }
    }

    release_spinlock(&threads_lock);
    return i;
}

void scheduler_unblock(struct thread* thread)
{
    thread->state = STATE_RUNNABLE;
    thread->wait_handle = NULL;
}

struct thread* scheduler_get_next_runnable_thread()
{
    struct thread* new_thread = NULL;

/*
    int runnable = 0;
    for (uint64 i = 0; i < MAX_THREADS; i ++) {
        if (sched_threads[i] != NULL) {
            if (sched_threads[i]->state == STATE_RUNNABLE)
                runnable++;
        }
    }

    if (runnable == 0) {
        return cpu_get_idle_thread();
    }*/

    while (1) {
        
        current_thread ++;
        if (current_thread > max_tid)
            current_thread = 0;

        new_thread = sched_threads[current_thread];

        if (new_thread == NULL) {
            continue;
        }

        if (new_thread->state != STATE_RUNNABLE) {
            continue;
        }

        break;
    }

    return new_thread;
}