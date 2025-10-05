#include "proc/syscalls.h"
#include "proc/process.h"
#include "cpu/cpu_local_x64.h"
#include "kairax/string.h"

int sys_sigpending(sigset_t *set, size_t sigsetsize)
{
    if (sigsetsize != sizeof(sigset_t)) {
        return -EINVAL;
    }

    struct thread* thread = cpu_get_current_thread();

    VALIDATE_USER_POINTER(thread->process, set, sigsetsize)

    memcpy(set, &thread->pending_signals, sigsetsize);

    return 0;
}

int sys_send_signal_pg(pid_t group, int signal)
{
    if (signal < 0 || signal > SIGNALS) {
        return -EINVAL;
    }

    process_list_send_signal_pg(group, signal);

    return 0;
}

int sys_send_signal(pid_t pid, int signal)
{
    if (signal < 0 || signal > SIGNALS) {
        return -EINVAL;
    }

    // Через отрицательное значение задается группа
    if (pid < 0)
    {
        return process_list_send_signal_pg(-pid, signal);
    }

    struct process* proc = process_get_by_id(pid);

    if (proc == NULL) {
        return -ESRCH;
    }

    struct thread* target = NULL;

    if (proc->type == OBJECT_TYPE_PROCESS) {
        target = proc->main_thread;
    } else {
        target = (struct thread*) proc;
    }

    if (target == NULL) {
        return -ESRCH;
    }

    return thread_send_signal(target, signal);
}

int sys_sigprocmask(int how, const sigset_t * set, sigset_t *oldset, size_t sigsetsize)
{
    if (sigsetsize != sizeof(sigset_t)) {
        return -EINVAL;
    }

    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, set, sigsetsize)
    if (oldset != NULL) {
        VALIDATE_USER_POINTER(process, oldset, sigsetsize)
        *oldset = process->blocked_signals;
    }

    switch (how) {
        case SIG_SETMASK:
            process->blocked_signals = *set;
            break;
        case SIG_BLOCK:
            process->blocked_signals |= *set;
            break;
        case SIG_UNBLOCK:
            process->blocked_signals &= ~(*set);
            break;
        default:
            return -ERROR_INVALID_VALUE;
    } 

    return 0;
}

int sys_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize)
{
    if (sigsetsize != sizeof(sigset_t)) {
        return -EINVAL;
    }

    if (signum < 0 || signum > SIGNALS) {
        return -EINVAL;
    }

    if (signum == SIGKILL || signum == SIGSTOP) {
        return -EINVAL;
    }

    struct process* process = cpu_get_current_thread()->process;

    if (act != NULL) 
    {
        VALIDATE_USER_POINTER(process, act, sizeof(struct sigaction));

        // Если есть SA_RESTORER то запишем адрес функции - трамплина
        if ((act->sa_flags & SA_RESTORER) == SA_RESTORER) 
            process->sighandle_trampoline = act->sa_trampoline;

        // Выбираем вариант функции обработчика
        int siginfo = (act->sa_flags & SA_SIGINFO) == SA_SIGINFO;

        process->sigactions[signum].handler = siginfo ? act->sa_sigaction : act->sa_handler;
        process->sigactions[signum].flags = act->sa_flags;
        process->sigactions[signum].sigmask = act->sa_mask;
    }

    if (oldact != NULL)
    {
        VALIDATE_USER_POINTER_PROTECTION(process, oldact, sizeof(struct sigaction), PAGE_PROTECTION_WRITE_ENABLE);

        oldact->sa_handler = process->sigactions[signum].handler;
        oldact->sa_flags = process->sigactions[signum].flags;
        oldact->sa_mask = process->sigactions[signum].sigmask;
    }

    return 0;
}

extern void arch_sigreturn();
int sys_sigreturn()
{
    arch_sigreturn();
}