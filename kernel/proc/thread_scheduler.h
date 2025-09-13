#ifndef _THREAD_SCHEDULER_H
#define _THREAD_SCHEDULER_H

#include "thread.h"

struct sched_wq {
    struct thread*  head;
    struct thread*  tail;
    size_t          size;
    int        since_balance;
};

void scheduler_sleep_intrusive(struct thread** head, struct thread** tail, spinlock_t* lock);
uint32_t scheduler_wakeup_intrusive(struct thread** head, struct thread** tail, spinlock_t* lock, uint32_t max);

/// @brief Блокировать поток
/// @param blocker блокировщик, по которму поток можно будет пробудить
/// @return 0 - при пробуждении из блокировщика, 1 - при пробуждении сигналом
int scheduler_sleep_on(struct blocker* blocker);
/// @brief Пробудить потоки по блокировщику
/// @param blocker блокировщик
/// @param max максимальное количество пробуждаемых потоков
/// @return количество пробужденных потоков
uint32_t scheduler_wake(struct blocker* blocker, uint32_t max);

int thread_intrusive_add(struct thread** head, struct thread** tail, struct thread* thread);
int thread_intrusive_remove(struct thread** head, struct thread** tail, struct thread* thread);

void wq_add_thread(struct sched_wq* wq, struct thread* thread);
void wq_remove_thread(struct sched_wq* wq, struct thread* thread);

void init_scheduler();

void scheduler_start();

void scheduler_add_thread(struct thread* thread);

void scheduler_remove_thread(struct thread* thread);

void scheduler_remove_process_threads(struct process* process, struct thread* despite);

struct thread* scheduler_get_next_runnable_thread();

void scheduler_yield(int save_context);

void scheduler_sleep(void* handle, spinlock_t* lock);

int scheduler_wakeup(void* handle, int max);
void scheduler_wakeup1(struct thread* thread);

void scheduler_unblock(struct thread* thread);


// Получить порядковый номер наиболее свободного процессора
uint32_t scheduler_get_less_loaded();

void cpu_put_thread(uint32_t icpu, struct thread* thread);

#endif