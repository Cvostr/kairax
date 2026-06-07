#include "sched.h"
#include <stddef.h>
#include "syscalls.h"
#include "errno.h"

int sched_yield (void)
{
    __set_errno(syscall_sched_yield());
}

int getcpu(unsigned int *cpu, unsigned int *node)
{
    __set_errno(syscall_getcpu(cpu, node));
}

int sched_getcpu(void)
{
    unsigned int cpu;
    int rc = getcpu(&cpu, NULL);
    if (rc == -1)
        return rc;

    return (int) cpu;
}