#include "idle.h"

struct process *idle_process = NULL;

extern void x64_idle_routine();

void init_idle_process()
{
    // Первоначальный процесс kterm
    idle_process = create_new_process(NULL);
    // Добавить в список и назначить pid
    process_add_to_list(idle_process);
}

struct thread* create_idle_thread()
{
    return create_kthread(idle_process, x64_idle_routine);
}