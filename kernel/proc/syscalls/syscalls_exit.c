#include "proc/syscalls.h"
#include "proc/process.h"
#include "kairax/intctl.h"

#include "cpu/cpu_local.h"

void sys_exit_process(int code)
{
    struct thread* thr = cpu_get_current_thread();
    struct process* process = thr->process;
    // Сохранить код возврата
    process->code = code;

    // Удалить другие потоки процесса из планировщика
    scheduler_remove_process_threads(process, thr);

    // Очистить ресурсы процесса
    process_free_resources(process);

    // Дикий кейс
    while (process->waiter == NULL)
    {
        scheduler_yield(TRUE);
    }

    // Данная операция должна выполниться атомарно
    disable_interrupts();

    // Удалить текущий оставшийся поток из планировщика
    scheduler_remove_process_threads(process, NULL);

    // После этого Данные оставшегося потока можно безопасно уничтожить
    thr->state = STATE_ZOMBIE;

    // Установить состояние zombie
    process->state = STATE_ZOMBIE;

    // Разбудить потоки, ждущие pid
    if (process->waiter) {
        scheduler_wakeup1(process->waiter);
    }
    
    scheduler_yield(FALSE);
}

void sys_exit_thread(int code)
{
    // Получить объект потока
    struct thread* thr = cpu_get_current_thread();
    thr->code = code;

    // Данная операция должна выполниться атомарно
    disable_interrupts();
    // Убрать поток из списка планировщика
    scheduler_remove_thread(thr);
    // После этого Данные потока можно безопасно уничтожить
    thread_become_zombie(thr);

    // Разбудить потоки, ждущие pid
    if (thr->waiter) {
        scheduler_wakeup1(thr->waiter);
    }

    // Выйти
    scheduler_yield(FALSE);
}