#include "threads.h"
#include "syscalls.h"
#include "errno.h"
#include "sys/wait.h"

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

void thrd_exit(int res)
{
    syscall_thread_exit(res);
}

int thrd_join(thrd_t thr, int* res)
{
    pid_t rc = waitpid(thr.tid, res, 0);
    if (rc == -1)
    {
        errno = -rc;
        return thrd_error;
    }

    return thrd_success;
}