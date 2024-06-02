#include "proc/syscalls.h"
#include "cpu/cpu_local_x64.h"
#include "proc/thread_scheduler.h"

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

int sys_futex(void* futex, int op, int val, const struct timespec *timeout)
{
    struct process* process = cpu_get_current_thread()->process;

    VALIDATE_USER_POINTER(process, futex, sizeof(int))

    uint64_t futex_physaddr = vm_get_physical_addr(process->vmemory_table, futex);

    if (op == FUTEX_WAIT) {
        if (*((int*)futex) != val) {
            return -EAGAIN;
        }

        scheduler_sleep(futex_physaddr, NULL);
       
    } else if (op == FUTEX_WAKE) {
        //printk(" WOKE UP  %i  ", futex_physaddr);
        return scheduler_wakeup(futex_physaddr, val);
    }

    return 0;
}