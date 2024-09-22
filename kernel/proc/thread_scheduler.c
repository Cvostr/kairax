#include "thread_scheduler.h"
#include "cpu/cpu_local_x64.h"

void init_scheduler()
{
   
}

#define DISABLE_INTS  int intsenab = check_interrupts(); disable_interrupts();
#define ENABLE_INTS  if (intsenab) enable_interrupts();

void thread_intrusive_add(struct thread** head, struct thread** tail, struct thread* thread)
{
    if (thread->prev != NULL || thread->next != NULL)
    {
        // Если поток уже в каком либо списке - выходим
        return;
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
}

void thread_intrusive_remove(struct thread** head, struct thread** tail, struct thread* thread)
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
}

uint32_t scheduler_sleep_intrusive(struct thread** head, struct thread** tail, spinlock_t* lock)
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
            
            DISABLE_INTS
            wq_add_thread(wq, thread);
            ENABLE_INTS

            scheduler_unblock(thread);

            unblocked ++;
        }

        thread = next;
    }

    if (lock != NULL)
        release_spinlock(lock);

    return unblocked;
}

void wq_add_thread(struct sched_wq* wq, struct thread* thread)
{
    thread_intrusive_add(&wq->head, &wq->tail, thread);
    wq->size++;
}

void wq_remove_thread(struct sched_wq* wq, struct thread* thread)
{
    thread_intrusive_remove(&wq->head, &wq->tail, thread);
    wq->size--;
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

    DISABLE_INTS
    wq_add_thread(wq, thread);
    thread->state = STATE_RUNNABLE;
    ENABLE_INTS
}

int scheduler_wakeup(void* handle, int max)
{
    int unblocked = 0;
    struct sched_wq* wq = cpu_get_wq();

    DISABLE_INTS

    struct thread* thread = wq->head;

    while (thread != NULL) {

        if (thread->state == STATE_INTERRUPTIBLE_SLEEP && thread->wait_handle == handle && unblocked < max) {
            scheduler_unblock(thread);
            unblocked ++;
        }

        thread = thread->next;
    }

    ENABLE_INTS
    
    return unblocked;
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
        return cpu_get_idle_thread();
    }

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