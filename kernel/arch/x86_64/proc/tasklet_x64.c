#include "proc/tasklet.h"
#include "proc/thread.h"
#include "cpu/cpu_local_x64.h"
#include "kairax/intctl.h"

struct process *tasklet_process = NULL;

void tasklet_schedule(struct tasklet* tasklet)
{
    struct tasklet_list* scheduled_tasklets = this_core->scheduled_tasklets;
    struct thread* thread = cpu_get_tasklet_thread();

    tasklet_schedule_generic(scheduled_tasklets, tasklet);

    __sync_add_and_fetch(&scheduled_tasklets->raise_cnt, 1);
    thread->state = STATE_RUNNABLE;
}

void x64_tasklet_routine()
{
    struct thread* thread = cpu_get_current_thread();
    struct tasklet_list* scheduled = this_core->scheduled_tasklets;

    while (1)
    {
        while (scheduled->raise_cnt == 0)
        {
            // Выходим (и, вероятно, блокируемся)
            disable_interrupts();
            // Если задачи не появились, то блокируемся
            thread->state = STATE_INTERRUPTIBLE_SLEEP;
            // Переходим к другому потоку
            scheduler_yield(TRUE);
        }  

        __sync_sub_and_fetch(&scheduled->raise_cnt, 1);

        // Выполнить тасклеты
        tasklet_list_execute(scheduled);
    }
}

void* create_tasklet_thread()
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
    // Запретить потоку переходить на другие ядра
    thr->balance_forbidden = TRUE;
    // Сразу же зададим ID процессора, на котором это исполняется
    thr->cpu = cpu_get_id();

    struct sched_wq* wq = this_core->wq;

    wq_add_thread(wq, thr);

    return thr;
}