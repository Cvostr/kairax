#include "thread.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "thread_scheduler.h"

struct thread* new_thread(struct process* process)
{
    struct thread* thread    = (struct thread*)kmalloc(sizeof(struct thread));
    if (thread == NULL) {
        return NULL;
    }

    memset(thread, 0, sizeof(struct thread));
    thread->process = process;
    thread->type = OBJECT_TYPE_THREAD;

    if (process->main_thread == NULL)
        process->main_thread = thread;

    return thread;
}

void thread_set_name(struct thread* thread, const char* name)
{
    strncpy(thread->name, name, PROCESS_NAME_MAX_LEN - 1);
}

void thread_set_sched_mode(struct thread* thread, int mode)
{
    thread->sched_mode = mode;
}

void thread_become_zombie(struct thread* thread)
{
    thread->state = STATE_ZOMBIE;
}

void thread_destroy(struct thread* thread)
{
    while (thread->state != STATE_ZOMBIE);

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

int thread_is_signal_blocked(struct thread* thread, int signal)
{
    sigset_t shifted = 1ULL << signal;
    int blocked = (thread->blocked_signals & shifted) == shifted;
    int nonmaskable = signal == SIGKILL || signal == SIGSTOP; 

    return (blocked && !nonmaskable);
}

int thread_send_signal(struct thread* thread, int signal)
{
    if (thread == NULL)
    {
        return -EINVAL;
    }

    sigset_t shifted = 1ULL << signal;

    struct process *proc = thread->process;
    struct proc_sigact* sigact = &proc->sigactions[signal];
    
    if (sigact_should_discard(sigact, signal) == TRUE)
        return 0;

    thread->pending_signals |= shifted;

    if (thread_is_signal_blocked(thread, signal) == FALSE)
    {
        if (thread->state == STATE_INTERRUPTIBLE_SLEEP)
        {
            thread->wake_reason = WAKE_SIGNAL;
            scheduler_wakeup(thread);
        }
    }

    return 0;
}

void thread_prepare_for_kill(struct thread* thread)
{
    thread->killing = TRUE;

    if (thread->state == STATE_INTERRUPTIBLE_SLEEP)
    {
        thread->wake_reason = WAKE_SIGNAL;
        scheduler_wakeup(thread);
    }
}