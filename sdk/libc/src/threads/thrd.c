#include "threads.h"
#include "syscalls.h"
#include "errno.h"

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    pid_t sysresult = syscall_create_thread(func, arg, 0);
    if (sysresult < 0)
    {
        errno = -sysresult;
        return thrd_error;
    }

    thr->tid = sysresult;
    thr->func = func;
    thr->arg = arg;

    return thrd_success;
}