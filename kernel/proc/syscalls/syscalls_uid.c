#include "proc/syscalls.h"
#include "proc/process.h"
#include "cpu/cpu_local_x64.h"

uid_t sys_getuid(void)
{
    struct process* process = cpu_get_current_thread()->process;
    return process->uid;
}

uid_t sys_geteuid(void)
{
    struct process* process = cpu_get_current_thread()->process;
    return process->euid;
}

int sys_setuid(uid_t uid)
{
    struct process* process = cpu_get_current_thread()->process;
    if (uid != process->euid) 
    {
        if (process->euid != 0) 
        {
            process->uid = 0;
            process->euid = 0;
        } else {
            return -EPERM;
        }
    }

    return 0;
}