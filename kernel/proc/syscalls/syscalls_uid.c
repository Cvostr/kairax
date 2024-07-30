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
        if (process->euid == 0) 
        {
            process->uid = uid;
            process->euid = uid;
        } else {
            return -EPERM;
        }
    }

    return 0;
}

gid_t sys_getgid(void)
{
    struct process* process = cpu_get_current_thread()->process;
    return process->gid;
}

gid_t sys_getegid(void)
{
    struct process* process = cpu_get_current_thread()->process;
    return process->egid;
}

int sys_setgid(gid_t gid)
{
    struct process* process = cpu_get_current_thread()->process;
    
    if (process->euid == 0) 
    {
        process->gid = gid;
        process->egid = gid;
    } else {
        return -EPERM;
    }

    return 0;
}