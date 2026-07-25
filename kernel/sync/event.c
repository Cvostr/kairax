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
    int irq_state;
    
    acquire_spinlock_irq(&ev->lock, &irq_state);

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
            current->sem_prev = NULL;
        } else {
            current->sem_prev = ev->last;
            ev->last->sem_next = current; 
        }

        ev->last = current;

        do
        {
            // Исключает поток из рабочей очереди текущего CPU
            // И ставит статус "блокирован"
            scheduler_prepare_sleep(current);

            // Снимаем спин
            release_spinlock(&ev->lock);
            // засыпаем
            scheduler_yield(TRUE);
            // проснулись, возвращаем блокировку
            acquire_spinlock_irq(&ev->lock, &irq_state);
        } while (ev->state == 0);

        if (current->sem_prev != NULL)
        {
            current->sem_prev->sem_next = current->sem_next;
        }
        else
        {
            ev->first = current->sem_next;
        }
    }

    // сбрасываем событие
    ev->state --;

    release_spinlock_irq(&ev->lock, irq_state);
}

void event_complete(struct event *ev)
{
    int irq_state;
    acquire_spinlock_irq(&ev->lock, &irq_state);

    // помечаем событие как завершенное
    ev->state ++;

    // будим потоки, если они есть
    struct thread *thread = ev->first;
    if (thread != NULL)
    {
        scheduler_wakeup(thread);
    }

    release_spinlock_irq(&ev->lock, irq_state);
}