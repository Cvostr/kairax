#include "event.h"
#include "proc/thread.h"
#include "cpu/cpu_local.h"
#include "kairax/intctl.h"

void event_init(struct event *ev, int state)
{
    ev->lock = 0;
    ev->state = state;

    ev->first = NULL;
    ev->last = NULL;
}

void event_reset(struct event *ev)
{
    ev->state = 0;
}

void event_wait(struct event *ev)
{
    struct thread *current = NULL;
    acquire_spinlock(&ev->lock);

    // проверяем, вдруг событие уже завершено
    if (ev->state == 0)
    {
        // Так как событие еще не было завершено, то засыпаем
        current = cpu_get_current_thread();

        // Зануляем адрес следующего потока в семафоре
        current->sem_next = NULL;

        // Добавление потока в список блокированных
        if (ev->first == NULL) {
            ev->first = current;
        } else {
            ev->last->sem_next = current; 
        }

        ev->last = current;

        while (ev->state == 0)
        {
            // Выключаем прерывания, так как дальнейшие действия надо выполнить атомарно
            disable_interrupts();

            // Исключает поток из рабочей очереди текущего CPU
            // И ставит статус "блокирован"
            scheduler_prepare_sleep(current);

            // Снимаем спин
            release_spinlock(&ev->lock);
            // засыпаем
            scheduler_yield(TRUE);
            // проснулись, возвращаем блокировку
            acquire_spinlock(&ev->lock);
        }
    }

    // сбрасываем событие
    ev->state --;

    release_spinlock(&ev->lock);
}

void event_complete(struct event *ev)
{
    acquire_spinlock(&ev->lock);

    struct thread *thread = ev->first;

    // помечаем событие как завершенное
    ev->state ++;

    // будим потоки, если они есть
    if (thread != NULL)
    {
        ev->first = thread->sem_next;
        scheduler_wakeup(thread);
    }

    release_spinlock(&ev->lock);
}