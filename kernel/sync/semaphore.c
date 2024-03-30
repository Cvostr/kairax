#include "semaphore.h"
#include "mem/kheap.h"
#include "string.h"
#include "proc/thread.h"
#include "proc/thread_scheduler.h"
#include "cpu/cpu_local_x64.h"

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
    while (sem->current > 0) {
        // Должны дождаться, пока все не отпустят семафор
        scheduler_sleep(sem, NULL);
    }
    acquire_spinlock(&sem->lock);
    kfree(sem);
}

void semaphore_acquire(struct semaphore* sem)
{
    acquire_spinlock(&sem->lock);

    if (sem->current < sem->max) {
        sem->current ++;
    } else {
        struct thread* current = cpu_get_current_thread();

        if (sem->first == NULL) {
            sem->first = current;
        } else {
            sem->last->next_blocked_thread = current; 
        }

        sem->last = current;

        // Блокируемся и выходим
        scheduler_sleep(NULL, &sem->lock);
    }

    release_spinlock(&sem->lock);
}

void semaphore_release(struct semaphore* sem)
{
    acquire_spinlock(&sem->lock);

    if (sem->first) {
        // Первый заблокированный поток
        struct thread* first_thread = sem->first;
        sem->first = first_thread->next_blocked_thread; 
        first_thread->next_blocked_thread = NULL;
        // Разблокировка потока
        scheduler_unblock(first_thread);
    } else {
        sem->current --;

        // Если все освободили семафор - пробуждаем потоки
        if (sem->current == 0) {
            scheduler_wakeup(sem, INT_MAX);
        }
    }

    release_spinlock(&sem->lock);
}