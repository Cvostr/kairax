#include "thread_scheduler.h"
#include "cpu/cpu_local_x64.h"

void init_scheduler()
{
   
}

void wq_add_thread(struct sched_wq* wq, struct thread* thread)
{
    if (!wq->head) {
        // Первого элемента не существует
        wq->head = thread;
    } else {
        wq->tail->next = thread;
        thread->prev = wq->tail;
    }

    wq->tail = thread;
}

void wq_remove_thread(struct sched_wq* wq, struct thread* thread)
{
    struct thread* prev = thread->prev;
    struct thread* next = thread->next;

    if (prev) {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }

    if (wq->head == thread) {
        wq->head = next;
        if (wq->head) wq->head->prev = NULL;
    }

    if (wq->tail == thread) {
        wq->tail = prev;
        if (wq->tail) wq->tail->next = NULL;
    }
}

void scheduler_add_thread(struct thread* thread)
{
    struct sched_wq* wq = cpu_get_wq();
    disable_interrupts();

    wq_add_thread(wq, thread);

    enable_interrupts();
}

void scheduler_remove_thread(struct thread* thread)
{
    struct sched_wq* wq = cpu_get_wq();
    disable_interrupts();
    
    wq_remove_thread(wq, thread);

    enable_interrupts();
}

void scheduler_remove_process_threads(struct process* process)
{
    // TODO: Ожидание выхода всех потоков из системных вызовов
    for (size_t i = 0; i < list_size(process->threads); i ++) {
        struct thread* thread = list_get(process->threads, i);
        scheduler_remove_thread(thread);
    }
}

void scheduler_sleep(void* handle, spinlock_t* lock)
{
    struct thread* thr = cpu_get_current_thread();

    if (lock != NULL)
        release_spinlock(lock);

    // Изменяем состояние - блокируемся
    thr->wait_handle = handle;
    thr->state = STATE_INTERRUPTIBLE_SLEEP;

    // Передача управления другому процессу
    scheduler_yield(TRUE);

    if (lock != NULL)
        // Блокируем спинлок
        acquire_spinlock(lock);
}

int scheduler_wakeup(void* handle, int max)
{
    int i = 0;
    struct sched_wq* wq = cpu_get_wq();
    disable_interrupts();

    struct thread* thread = wq->head;

    while (thread != NULL) {

        if (thread->state == STATE_INTERRUPTIBLE_SLEEP && thread->wait_handle == handle && i < max) {
            scheduler_unblock(thread);
            i ++;
        }

        thread = thread->next;
    }

    enable_interrupts();
    return i;
}

void scheduler_unblock(struct thread* thread)
{
    thread->state = STATE_RUNNABLE;
    thread->wait_handle = NULL;
}

struct thread* scheduler_get_next_runnable_thread()
{
    struct sched_wq* wq = cpu_get_wq();
    struct thread* thread = wq->head;


    int runnable = 0;
    while (thread != NULL) {
        if (thread->state == STATE_RUNNABLE) {
            runnable = 1;
            break;
        }
        thread = thread->next;
    }
    if (runnable == 0) {
        return cpu_get_idle_thread();
    }


    thread = cpu_get_current_thread();

    if (thread == NULL) {
        return wq->head;
    }

    while (1) {

        thread = thread->next;

        if (thread == NULL) {
            thread = wq->head;
        }

        if (thread->state != STATE_RUNNABLE) {
            continue;
        }

        break;
    }

    return thread;
}