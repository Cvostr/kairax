#include "sys/wait.h"
#include "syscalls.h"
#include "errno.h"

pid_t wait(int *status)
{
    return waitpid(-1, status, 0);
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    __set_errno(syscall_wait(0, pid, status, options));
}