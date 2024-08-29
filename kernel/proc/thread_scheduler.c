#include "thread_scheduler.h"
#include "cpu/cpu_local_x64.h"

struct sched_wq thread_queue;

void init_scheduler()
{
   
}

void scheduler_add_thread(struct thread* thread)
{
    disable_interrupts();

    if (!thread_queue.head) {
        // Первого элемента не существует
        thread_queue.head = thread;
    } else {
        thread_queue.tail->next = thread;
        thread->prev = thread_queue.tail;
    }

    thread_queue.tail = thread;

    enable_interrupts();
}

void scheduler_remove_thread(struct thread* thread)
{
    disable_interrupts();
    
    struct thread* prev = thread->prev;
    struct thread* next = thread->next;

    if (prev) {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }

    if (thread_queue.head == thread) {
        thread_queue.head = next;
        if (thread_queue.head) thread_queue.head->prev = NULL;
    }

    if (thread_queue.tail == thread) {
        thread_queue.tail = prev;
        if (thread_queue.tail) thread_queue.tail->next = NULL;
    }

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
    disable_interrupts();

    struct thread* thread = thread_queue.head;

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
    struct thread* thread = thread_queue.head;


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
        return thread_queue.head;
    }

    while (1) {

        thread = thread->next;

        if (thread == NULL) {
            thread = thread_queue.head;
        }

        if (thread->state != STATE_RUNNABLE) {
            continue;
        }

        break;
    }

    return thread;
}