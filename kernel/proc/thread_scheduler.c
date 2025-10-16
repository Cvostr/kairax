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

int scheduler_sleep_on(struct blocker* blocker)
{
    struct thread* thr = cpu_get_current_thread();

    thr->blocker = blocker;
    scheduler_sleep_intrusive(&blocker->head, &blocker->tail, &blocker->lock);

    if (thr->sleep_interrupted == TRUE)
    {
        thr->sleep_interrupted = FALSE;
        return 1;
    }

    return 0;
}

uint32_t scheduler_wake(struct blocker* blocker, uint32_t max)
{
    return scheduler_wakeup_intrusive(&blocker->head, &blocker->tail, &blocker->lock, max);
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
            thread->blocker = NULL;
            
            if (thread->state == STATE_INTERRUPTIBLE_SLEEP || thread->state == STATE_UNINTERRUPTIBLE_SLEEP)
            {
                DISABLE_INTS
                wq_add_thread(wq, thread);
                scheduler_unblock(thread);
                ENABLE_INTS

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

void scheduler_sleep(void* handle, spinlock_t* lock)
{
    printk("Deprecated!\n");
}

int scheduler_sleep1()
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

    if (thr->sleep_interrupted == TRUE)
    {
        thr->sleep_interrupted = FALSE;
        return 1;
    }

    return 0;
}

void scheduler_wakeup1(struct thread* thread)
{
    struct sched_wq* wq = cpu_get_wq();

    if (thread->state == STATE_INTERRUPTIBLE_SLEEP || thread->state == STATE_UNINTERRUPTIBLE_SLEEP)
    {
        // Если поток был усыплен на таймере
        if (thread->timer != NULL)
        {
            unregister_event_timer(thread->timer);
        }

        if (thread->blocker)
        {
            thread_intrusive_remove(&thread->blocker->head, &thread->blocker->tail, thread);
            thread->blocker = NULL;
        }

        DISABLE_INTS
        wq_add_thread(wq, thread);
        scheduler_unblock(thread);
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
    thread->timer = NULL;
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

    // Счетчик проверенных потоков
    int checked_threads = 0;

    while (1) {

        if (checked_threads == wq->size)
        {
            // Все потоки в очереди заблокированы, возвращаем IDLE задачу
            return cpu_get_idle_thread();
        }

        thread = thread->next;

        if (thread == NULL) {
            thread = wq->head;
        }

        if (thread->state != STATE_RUNNABLE) {
            checked_threads++;
            continue;
        }

        break;
    }

    return thread;
}