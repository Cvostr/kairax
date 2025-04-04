#include "process_list.h"
#include "process.h"
#include "thread.h"
#include "sync/spinlock.h"

#define MAX_PROCESSES 3000

struct process* processes[MAX_PROCESSES];
spinlock_t process_lock = 0;

pid_t process_add_to_list(struct process* process)
{
    pid_t pid;
    acquire_spinlock(&process_lock);

    for (pid = 0; pid < MAX_PROCESSES; pid ++) {
        if (processes[pid] == NULL) {
            processes[pid] = process;
            process->pid = pid;

            break;
        }
    }
    
    release_spinlock(&process_lock);

    return pid;
}

void get_process_count(size_t *processes_n, size_t *threads)
{
    pid_t pid;

    *processes_n = 0;
    *threads = 0;

    acquire_spinlock(&process_lock);

    for (pid = 0; pid < MAX_PROCESSES; pid ++) 
    {
        struct process* proc = processes[pid];
        if (proc != NULL) 
        {
            switch (proc->type) {
                case OBJECT_TYPE_PROCESS:
                    (*processes_n) += 1;
                    break;
                case OBJECT_TYPE_THREAD:
                    (*threads) += 1;
                    break;
            }
        }
    }
    
    release_spinlock(&process_lock);
}

struct process* process_get_by_id(pid_t id)
{
    return processes[id];
}

void process_remove_from_list(struct process* process)
{
    acquire_spinlock(&process_lock);
    
    if (processes[process->pid] == process) {
        processes[process->pid] = NULL;
    }

    release_spinlock(&process_lock);
}

int process_list_is_dentry_used_as_cwd(struct dentry* dentry)
{
    int rc = 0;
    acquire_spinlock(&process_lock);

    for (int pid = 0; pid < MAX_PROCESSES; pid ++) {

        struct process* proc = processes[pid];

        if (proc != NULL) {
            
            if (proc->type != OBJECT_TYPE_PROCESS) {
                continue;
            }

            if (proc->pwd == dentry) {
                rc = 1;
                break;
            }                
        }
    }

    release_spinlock(&process_lock);
    return rc;
}

void plist_debug() 
{
    for (int pid = 0; pid < MAX_PROCESSES; pid ++) {

        struct process* proc = processes[pid];

        if (proc != NULL) {
            
            printk("\"%s\": pid: %i State %i ", proc->name, proc->pid, proc->state);      
            if (proc->type == OBJECT_TYPE_THREAD) {
                struct thread* thr = (struct thread*) proc;
                printk("CPU %i ", thr->cpu);
            }   
            printk("\n");  
        }
    }
}