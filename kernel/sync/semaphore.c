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
    acquire_spinlock(&sem->lock);

    if (sem->current < sem->max) {
        sem->current ++;
    } else {
        current = cpu_get_current_thread();

        if (sem->first == NULL) {
            sem->first = current;
        } else {
            sem->last->next = current; 
        }

        sem->last = current;

        // Блокируемся и выходим
        release_spinlock(&sem->lock);
        return scheduler_sleep1();
    }

    release_spinlock(&sem->lock);
    return 0;
}

void semaphore_release(struct semaphore* sem)
{
    acquire_spinlock(&sem->lock);

    // Первый заблокированный поток
    struct thread* first_thread = sem->first;

    if (first_thread) 
    {
        sem->first = first_thread->next; 
        first_thread->next = NULL;

        release_spinlock(&sem->lock);
        // Разблокировка потока
        scheduler_wakeup1(first_thread);
        return;
    } else {
        sem->current --;
    }

    release_spinlock(&sem->lock);
}