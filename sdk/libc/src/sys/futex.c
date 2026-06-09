#include "sys/futex.h"
#include "syscalls.h"
#include "errno.h"

int futex(int* addr, int op, int val, const struct timespec* timeout)
{
    __sys_ret_set_errno(syscall_futex(addr, op, val, timeout));
}