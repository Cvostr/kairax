#include "thread.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "thread_scheduler.h"

int last_id = 0;

struct thread* new_thread(struct process* process)
{
    struct thread* thread    = (struct thread*)kmalloc(sizeof(struct thread));
    memset(thread, 0, sizeof(struct thread));
    thread->id          = last_id++;
    thread->process     = (process);
    return thread;
}

int thread_sleep(struct thread* thread, uint64_t time)
{
    thread->state = THREAD_UNINTERRUPTIBLE; // Ожидающий системный вызов
    for (uint64_t i = 0; i < time; i ++) {
        scheduler_yield();
    }
    thread->state = THREAD_RUNNING;
}