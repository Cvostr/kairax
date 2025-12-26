#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include "atomic.h"
#include "spinlock.h"
#include "proc/blocker.h"

struct thread;

struct semaphore {
    spinlock_t      lock;
    int             max;
    atomic_t        current;
    struct thread*  first;
    struct thread*  last;
};

struct semaphore* new_semaphore(int max);
struct semaphore* new_mutex();
void free_semaphore(struct semaphore* sem);
void semaphore_init(struct semaphore* sem, int max);
int semaphore_acquire(struct semaphore* sem);
void semaphore_release(struct semaphore* sem);

#endif