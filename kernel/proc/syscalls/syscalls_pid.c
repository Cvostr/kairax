#include "cpu/cpu_local_x64.h"
#include "proc/process.h"

pid_t sys_get_process_id()
{
    struct process* process = cpu_get_current_thread()->process;
    return process->pid;
}

pid_t sys_get_parent_process_id()
{
    struct process* process = cpu_get_current_thread()->process;
    struct process* parent = process->parent;

    if (parent == NULL) {
        // ну мало ли
        return -1;
    }

    return parent->pid;
}

pid_t sys_get_thread_id()
{
    struct thread* current_thread = cpu_get_current_thread();
    return current_thread->id;
}

int sys_setpgid(pid_t pid, pid_t pgid)
{
    struct process* proc = NULL;

    if (pid < 0 || pgid < 0)
    {
        return -EINVAL;
    } 
    else if (pid == 0) 
    {
        proc = cpu_get_current_thread()->process;
    }
    else
    {
        proc = process_get_by_id(pid);
        if (proc == NULL) 
        {
            return -ESRCH;
        }
    }

    if (pgid == 0)
    {
        pgid = proc->pid;
    }

    proc->process_group = pgid;

    return 0;
}

pid_t sys_getpgid(pid_t pid)
{
    struct process* proc = NULL;

    if (pid < 0)
    {
        return -EINVAL;
    } 
    else if (pid == 0) 
    {
        proc = cpu_get_current_thread()->process;
    }
    else
    {
        proc = process_get_by_id(pid);
        if (proc == NULL) 
        {
            return -ESRCH;
        }
    }

    return proc->process_group;
}