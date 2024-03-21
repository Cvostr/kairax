#include "sched.h"
#include "syscalls.h"
#include "errno.h"

int sched_yield (void)
{
    __set_errno(syscall_sched_yield());
}