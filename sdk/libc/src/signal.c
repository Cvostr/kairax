#include "signal.h"
#include "unistd.h"
#include "errno.h"
#include "syscalls.h"

int raise(int sig)
{
    return kill(getpid(), sig);
}

int kill(pid_t pid, int sig)
{
    __set_errno(syscall_kill(pid, sig));
}

int sigaddset (sigset_t *set, int sig)
{
    if ((sig < 1)) {
        errno = EINVAL;
        return -1;
    }

    *set |= (1ULL << sig);

    return 0; 
}

int sigdelset(sigset_t *set, int sig)
{
    if ((sig < 1)) {
        errno = EINVAL;
        return -1;
    }
    
    *set &= ~(1ULL << sig);

    return 0; 
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    __set_errno(syscall_sigprocmask(how, set, oldset, sizeof(sigset_t)));
}

int sigpending(sigset_t *set)
{
    __set_errno(syscall_sigpending(set, sizeof(sigset_t)));
}