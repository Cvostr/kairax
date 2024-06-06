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