#include "idle.h"

struct process *idle_process = NULL;

extern void x64_idle_routine();

void init_idle_process()
{
    // Родительский процесс для потоков idle
    idle_process = create_new_process(NULL);
    process_set_name(idle_process, "idle process");
    // Добавить в список и назначить pid
    process_add_to_list(idle_process);
}

struct thread* create_idle_thread()
{
    return create_kthread(idle_process, x64_idle_routine, NULL);
}