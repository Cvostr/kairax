#include "proc/syscalls.h"
#include "proc/process.h"
#include "kairax/intctl.h"

#include "cpu/cpu_local.h"

void exit_process(int code)
{
    struct thread* thr = cpu_get_current_thread();
    struct process* process = thr->process;
    struct process* parent_process = process->parent;
    // Сохранить код возврата
    process->code = code;

    // Безопасно останавливаем работу других потоков, если они были
    for (size_t i = 0; i < list_size(process->threads); i ++) 
    {
        struct thread* thread = list_get(process->threads, i);

        // Удаляем таймер для alarm из списка
        // Он вполне мог там застрять, если процесс завершается до того, как сработает alarm
        unregister_event_timer(&thread->alarm_timer);

        if (thread != thr) 
        {
            thread_prepare_for_kill(thread);
            while (thread->state != STATE_ZOMBIE);
        }
    }

    // Очистить ресурсы процесса
    process_free_resources(process);

    // Установить состояние zombie
    process->state = STATE_ZOMBIE;

    // Разбудить потоки, ждущие pid
    scheduler_wake(&parent_process->wait_blocker, INT_MAX);

    // Каждая из следующих операций приведет к тому, что поток больше не будет запускаться
    // Но они должны быть выполнены все
    // поэтому их надо делать с выключенными прерываниями
    disable_interrupts();

    // Пометим поток как зомби, чтобы планировщик не пытался его исполнить
    thr->state = STATE_ZOMBIE;

    // Удалить текущий оставшийся поток из планировщика
    scheduler_remove_thread(thr);

    scheduler_yield(FALSE);
}

void sys_exit_process(int code)
{
    exit_process(MAKEEXITCODE(code));
}

void sys_exit_thread(int code)
{
    // Получить объект потока
    struct thread* thr = cpu_get_current_thread();
    struct process* process = thr->process;

    thr->code = MAKEEXITCODE(code);

    // Если это последний поток, то убиваем уже весь процесс
    if (list_size(process->threads) == 1)
    {
        exit_process(thr->code);
    }

    // Уничтожить данные потока
    thread_clear_stack_tls(thr);

    // Удаляем таймер для alarm из списка
    // Он вполне мог там застрять, если процесс завершается до того, как сработает alarm
    unregister_event_timer(&thr->alarm_timer);

    // Данная операция должна выполниться атомарно
    disable_interrupts();
    // Убрать поток из списка планировщика
    scheduler_remove_thread(thr);
    // После этого сделать поток зомби
    thread_become_zombie(thr);

    // Разбудить потоки, ждущие pid
    scheduler_wake(&process->wait_blocker, INT_MAX);

    // Выйти
    scheduler_yield(FALSE);
}