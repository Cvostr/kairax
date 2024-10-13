#include "thread_scheduler.h"
#include "cpu/cpu_local_x64.h"

void init_scheduler()
{
   
}

#define DISABLE_INTS  int intsenab = check_interrupts(); disable_interrupts();
#define ENABLE_INTS  if (intsenab) enable_interrupts();

int thread_intrusive_add(struct thread** head, struct thread** tail, struct thread* thread)
{
    if (thread->prev != NULL || thread->next != NULL) {
        // Если поток уже в каком либо списке - выходим
        return -1;
    }

    if (!(*head)) {
        // Первого элемента не существует
        *head = thread;
        thread->prev = NULL;
    } else {
        (*tail)->next = thread;
        thread->prev = (*tail);
    }

    // Добавляем в конец - следующего нет
    thread->next = NULL;

    *tail = thread;

    return 0;
}

int thread_intrusive_remove(struct thread** head, struct thread** tail, struct thread* thread)
{
    struct thread* prev = thread->prev;
    struct thread* next = thread->next;

    if (prev) {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }

    if (*head == thread) {
        *head = next;
        if (*head) (*head)->prev = NULL;
    }

    if (*tail == thread) {
        *tail = prev;
        if (*tail) (*tail)->next = NULL;
    }

    thread->next = NULL;
    thread->prev = NULL;

    return 0;
}

void scheduler_sleep_intrusive(struct thread** head, struct thread** tail, spinlock_t* lock)
{
    struct thread* thr = cpu_get_current_thread();
    struct sched_wq* wq = cpu_get_wq();

    if (lock != NULL)
        acquire_spinlock(lock);

    DISABLE_INTS

    // Изменяем состояние - блокируемся
    thr->state = STATE_INTERRUPTIBLE_SLEEP;

    // удалить из очереди текущего ЦП
    wq_remove_thread(wq, thr);

    // Добавить в очередь ожидания
    thread_intrusive_add(head, tail, thr);

    if (lock != NULL)
        // Блокируем спинлок
        release_spinlock(lock);

    // Передача управления другому процессу
    scheduler_yield(TRUE);
}

uint32_t scheduler_wakeup_intrusive(struct thread** head, struct thread** tail, spinlock_t* lock, uint32_t max)
{
    struct sched_wq* wq = cpu_get_wq();

    uint32_t unblocked = 0;
    struct thread* thread = *head;
    struct thread* next = thread;

    if (lock != NULL)
        acquire_spinlock(lock);

    while (thread != NULL) {

        next = next->next;

        if (unblocked < max) {
            
            thread_intrusive_remove(head, tail, thread);
            
            if (thread->state == STATE_INTERRUPTIBLE_SLEEP || thread->state == STATE_UNINTERRUPTIBLE_SLEEP)
            {
                DISABLE_INTS
                wq_add_thread(wq, thread);
                ENABLE_INTS

                scheduler_unblock(thread);

                unblocked ++;
            }
        }

        thread = next;
    }

    if (lock != NULL)
        release_spinlock(lock);

    return unblocked;
}

void wq_add_thread(struct sched_wq* wq, struct thread* thread)
{
    if (thread->in_queue == 1) {
        return;
    }

    if (thread_intrusive_add(&wq->head, &wq->tail, thread) == 0) {
        wq->size++;
        thread->in_queue = 1;
    }
}

void wq_remove_thread(struct sched_wq* wq, struct thread* thread)
{
    if (wq->size == 0) {
        printk("wq_remove_thread(): wq->size == 0, pid %i\n", thread->id);
        return;
    }

    if (thread->in_queue == 0) {
        return;
    }

    if (thread_intrusive_remove(&wq->head, &wq->tail, thread) == 0) {
        wq->size--;
        thread->in_queue = 0;
    }
}

void scheduler_add_thread(struct thread* thread)
{
    struct sched_wq* wq = cpu_get_wq();
    DISABLE_INTS

    wq_add_thread(wq, thread);

    ENABLE_INTS
}

void scheduler_remove_thread(struct thread* thread)
{
    struct sched_wq* wq = cpu_get_wq();
    DISABLE_INTS
    
    wq_remove_thread(wq, thread);

    ENABLE_INTS
}

void scheduler_remove_process_threads(struct process* process, struct thread* despite)
{
    // TODO: Ожидание выхода всех потоков из системных вызовов
    for (size_t i = 0; i < list_size(process->threads); i ++) {
        struct thread* thread = list_get(process->threads, i);
        if (thread != despite) {
            scheduler_remove_thread(thread);
            thread->state = STATE_ZOMBIE;
        }
    }
}

void scheduler_sleep(void* handle, spinlock_t* lock)
{
    printk("Deprecated!\n");
}

void scheduler_sleep1()
{
    struct sched_wq* wq = cpu_get_wq();
    struct thread* thr = cpu_get_current_thread();

    DISABLE_INTS

    // Изменяем состояние - блокируемся
    thr->state = STATE_INTERRUPTIBLE_SLEEP;

    // удалить из очереди текущего ЦП
    wq_remove_thread(wq, thr);

    // Передача управления другому процессу
    scheduler_yield(TRUE);
}

void scheduler_wakeup1(struct thread* thread)
{
    struct sched_wq* wq = cpu_get_wq();

    if (thread->state == STATE_INTERRUPTIBLE_SLEEP || thread->state == STATE_UNINTERRUPTIBLE_SLEEP)
    {
        DISABLE_INTS
        wq_add_thread(wq, thread);
        thread->state = STATE_RUNNABLE;
        thread->sleep_raiser = NULL;
        ENABLE_INTS
    }
}

int scheduler_wakeup(void* handle, int max)
{
    printk("Deprecated!\n");
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

    if (wq == NULL || wq->size == 0)
    {
        // Очередь пуста, возвращаем IDLE задачу
        return cpu_get_idle_thread();
    }

    thread = cpu_get_current_thread();

    if (thread == NULL) 
    {
        // Достигли конца очереди, возвращаемся на начало
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