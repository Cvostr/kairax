#include "proc/syscalls.h"
#include "cpu/cpu_local_x64.h"
#include "proc/process.h"

int sys_dup(int oldfd)
{
    struct process* process = cpu_get_current_thread()->process;

    struct file* file = process_get_file(process, oldfd);

    if (file == NULL)
    {
        return -ERROR_BAD_FD;
    }

    return process_add_file(process, file);
}

int sys_dup2(int oldfd, int newfd)
{
    if (oldfd == newfd)
    {
        return 0;
    }
    
    struct process* process = cpu_get_current_thread()->process;

    struct file* file = process_get_file(process, oldfd);

    if (file == NULL)
    {
        return -ERROR_BAD_FD;
    }

    return process_add_file_at(process, file, newfd);
}