#include "proc/process.h"
#include "proc/thread.h"
#include "proc/syscalls.h"
#include "cpu/cpu_local.h"

typedef unsigned long rlim_t;

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

#define RLIMIT_STACK	3
#define RLIMIT_NOFILE	7

int sys_getrlimit(int resource, struct rlimit *rlim)
{
    struct process* process = cpu_get_current_thread()->process;

    VALIDATE_USER_POINTER_PROTECTION(process, rlim, sizeof(struct rlimit), PAGE_PROTECTION_WRITE_ENABLE)

    switch (resource)
    {
    case RLIMIT_NOFILE:
        rlim->rlim_cur = MAX_DESCRIPTORS;
        rlim->rlim_max = MAX_DESCRIPTORS;
        break;
    case RLIMIT_STACK:
        rlim->rlim_cur = MAX_STACK_SIZE;
        rlim->rlim_max = MAX_STACK_SIZE;
        break;
    default:
        return -EINVAL;
    }

    return 0;
}
