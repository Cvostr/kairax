#include "thread.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "thread_scheduler.h"

struct thread* new_thread(struct process* process)
{
    struct thread* thread    = (struct thread*)kmalloc(sizeof(struct thread));
    memset(thread, 0, sizeof(struct thread));
    thread->process = process;
    thread->type = OBJECT_TYPE_THREAD;
    return thread;
}

void thread_become_zombie(struct thread* thread)
{
    thread->state = STATE_ZOMBIE;
}

void thread_destroy(struct thread* thread)
{
    // освободить страницу с стеком ядра
    pmm_free_page(V2P(thread->kernel_stack_ptr));
    kfree(thread);
}