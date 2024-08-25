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