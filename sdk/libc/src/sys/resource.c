#include <sys/resource.h>
#include "errno.h"
#include "syscalls.h"

int getrlimit(int resource, struct rlimit *rlim)
{
    __sys_ret_set_errno(syscall_getrlimit(resource, rlim));
}