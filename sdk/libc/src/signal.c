#include "signal.h"
#include "unistd.h"
#include "errno.h"
#include "syscalls.h"
#include "string.h"

extern void __sig_trampoline();

int raise(int sig)
{
    return kill(getpid(), sig);
}

int kill(pid_t pid, int sig)
{
    __set_errno(syscall_kill(pid, sig));
}

int killpg(pid_t pgrp, int sig)
{
    return kill(-pgrp, sig);
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

int sigfillset(sigset_t *set) 
{
	memset(set, 0xFF, sizeof(sigset_t));
	return 0;
}

int sigemptyset(sigset_t *set) 
{
    memset(set, 0, sizeof(sigset_t));
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

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    __set_errno(syscall_sigaction(signum, act, oldact, sizeof(sigset_t)));
}

sighandler_t signal(int signum, sighandler_t action)
{
    struct sigaction sact, oldact;
    sact.sa_restorer = __sig_trampoline;
    sact.sa_flags = 0x04000000;
    sact.sa_handler = action;
    sigemptyset(&sact.sa_mask);
    
    if (sigaddset(&sact.sa_mask, signum) != 0)
        return SIG_ERR;

    if (sigaction(signum, &sact, &oldact) != 0)
        return SIG_ERR;

    return oldact.sa_handler;
}