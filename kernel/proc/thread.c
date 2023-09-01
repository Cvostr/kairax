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