#include "proc/syscalls.h"
#include "proc/process.h"
#include "kairax/intctl.h"

#include "cpu/cpu_local_x64.h"

void sys_exit_process(int code)
{
    struct thread* thr = cpu_get_current_thread();
    struct process* process = thr->process;
    // Сохранить код возврата
    process->code = code;

    // Удалить другие потоки процесса из планировщика
    scheduler_remove_process_threads(process, cpu_get_current_thread());

    // Очистить процесс, сделать его зомби
    process_become_zombie(process);

    // Разбудить потоки, ждущие pid
    if (process->waiter) {
        scheduler_wakeup1(process->waiter);
    }

    // Данная операция должна выполниться атомарно
    disable_interrupts();

    // Удалить текущий оставшийся поток из планировщика
    scheduler_remove_process_threads(process, NULL);

    // После этого Данные оставшегося потока можно безопасно уничтожить
    thr->state = STATE_ZOMBIE;
    
    scheduler_yield(FALSE);
}

void sys_exit_thread(int code)
{
    // Получить объект потока
    struct thread* thr = cpu_get_current_thread();
    thr->code = code;

    // Разбудить потоки, ждущие pid
    if (thr->waiter) {
        scheduler_wakeup1(thr->waiter);
    }

    // Данная операция должна выполниться атомарно
    disable_interrupts();
    // Убрать поток из списка планировщика
    scheduler_remove_thread(thr);
    // После этого Данные потока можно безопасно уничтожить
    thread_become_zombie(thr);

    // Выйти
    scheduler_yield(FALSE);
}