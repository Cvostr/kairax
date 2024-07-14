#include "proc/syscalls.h"
#include "proc/process.h"
#include "cpu/cpu_local_x64.h"
#include "kairax/string.h"

int sys_sigpending(sigset_t *set, size_t sigsetsize)
{
    if (sigsetsize != sizeof(sigset_t)) {
        return -EINVAL;
    }

    struct process* process = cpu_get_current_thread()->process;

    VALIDATE_USER_POINTER(process, set, sigsetsize)

    memcpy(set, process->pending_signals, sigsetsize);

    return 0;
}

int sys_send_signal(pid_t pid, int signal)
{
    if (signal < 0 || signal > SIGNALS) {
        return -EINVAL;
    }

    struct process* proc = process_get_by_id(pid);

    if (proc == NULL) {
        return -ESRCH;
    }

    if (proc->type != OBJECT_TYPE_PROCESS) {
        return -1;
    }

    return process_send_signal(proc, signal);
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

int sys_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    if (signum < 0 || signum > SIGNALS) {
        return -EINVAL;
    }

    if (signum == SIGKILL || signum == SIGSTOP) {
        return -EINVAL;
    }

    struct process* process = cpu_get_current_thread()->process;

    if (act != NULL) {
        VALIDATE_USER_POINTER(process, act, sizeof(struct sigaction));

        if ((act->sa_flags & SA_RESTORER) == SA_RESTORER) 
            process->sighandle_trampoline = act->sa_trampoline;
    }

    return 0;
}

int sys_sigreturn()
{
    return 0; // Should return ?
}