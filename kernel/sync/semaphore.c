#include "semaphore.h"
#include "mem/kheap.h"
#include "string.h"
#include "proc/thread.h"
#include "proc/thread_scheduler.h"
#include "kairax/intctl.h"
#include "cpu/cpu_local.h"

struct semaphore* new_semaphore(int max)
{
    struct semaphore *sema = (struct semaphore*) kmalloc(sizeof(struct semaphore));
    memset(sema, 0, sizeof(struct semaphore));
    sema->max = max;
    return sema;
}

struct semaphore* new_mutex()
{
    return new_semaphore(1);
}

void free_semaphore(struct semaphore* sem)
{
    while (sem->current.counter > 0) {

    }

    acquire_spinlock(&sem->lock);
    kfree(sem);
}

void semaphore_init(struct semaphore* sem, int max)
{
    memset(sem, 0, sizeof(struct semaphore));
    sem->max = max;
}

int semaphore_acquire(struct semaphore* sem)
{
    struct thread  *current;

    if (atomic_inc_and_get(&sem->current) > sem->max) 
    {
        // Так как семафором владеют больше потоков, чем можно
        // То засыпаем
        current = cpu_get_current_thread();

        // Сначала блокируем спинлок, потому что будем изменять внутреннее состояние семафора
        acquire_spinlock(&sem->lock);

        // Выключаем прерывания, так как дальнейшие действия надо выполнить атомарно
        disable_interrupts();

        // Исключает поток из рабочей очереди тякущего CPU
        // И ставит статус "блокирован"
        scheduler_prepare_sleep(current);

        // Добавление потока в очередь блокированных
        // Делаем это именно сейчас, потому что semaphore_release() ожидает, что потоки в очереди уже блокированы
        if (sem->first == NULL) {
            sem->first = current;
        } else {
            sem->last->next = current; 
        }

        sem->last = current;

        // Снимаем спин и засыпаем
        release_spinlock(&sem->lock);
        scheduler_yield(TRUE);

        return scheduler_get_wake_reason(current);
    }

    return 0;
}

void semaphore_release(struct semaphore* sem)
{
    // Первый заблокированный поток
    struct thread* first_thread = NULL;

    if (__sync_sub_and_fetch(&sem->current.counter, 1) >= sem->max)
    {
        while (first_thread == NULL)
        {
            // Извлекаем первый блокированный поток
            acquire_spinlock(&sem->lock);
            first_thread = sem->first;
            if (first_thread) 
            {
                sem->first = first_thread->next; 
                first_thread->next = NULL;
            }
            release_spinlock(&sem->lock);
        }

        // Разблокировка потока
        scheduler_wakeup1(first_thread);
    }
}