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

    if (process->main_thread == NULL)
        process->main_thread = thread;

    return thread;
}

void thread_become_zombie(struct thread* thread)
{
    thread->state = STATE_ZOMBIE;
}

void thread_destroy(struct thread* thread)
{
#ifdef X86_64
    if (thread->fpu_context)
    {
        pmm_free_page(V2P(thread->fpu_context));
    }
#endif
    // освободить страницу с стеком ядра
    pmm_free_page(V2P(thread->kernel_stack_ptr));
    kfree(thread);
}

int thread_send_signal(struct thread* thread, int signal)
{
    sigset_t shifted = 1 << signal;

    int blocked = (thread->process->blocked_signals & shifted) == shifted;
    int nonmaskable = signal == SIGKILL || signal == SIGSTOP; 

    if (blocked && !nonmaskable) {
        return -1;
    }

    thread->pending_signals |= shifted;

    if (thread->state == STATE_INTERRUPTIBLE_SLEEP)
        scheduler_unblock(thread);

    return 0;
}