#include "proc/tasklet.h"
#include "proc/thread.h"
#include "cpu/cpu_local_x64.h"

struct process *tasklet_process = NULL;

void tasklet_schedule(struct tasklet* tasklet)
{
    tasklet_schedule_generic(this_core->scheduled_tasklets, tasklet);
}

void x64_tasklet_routine()
{
    struct tasklet_list* scheduled = this_core->scheduled_tasklets;
    while (1)
    {
        tasklet_list_execute(scheduled);
    }
}

void create_tasklet_thread()
{
    if (tasklet_process == NULL)
    {
        // Родительский процесс для потоков tasklet
        tasklet_process = create_new_process(NULL);
        process_set_name(tasklet_process, "tasklet process");
        // Добавить в список и назначить pid
        process_add_to_list(tasklet_process);
    }

    struct thread* thr = create_kthread(tasklet_process, x64_tasklet_routine, NULL);

    struct sched_wq* wq = this_core->wq;

    wq_add_thread(wq, thr);
}