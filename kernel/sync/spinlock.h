#ifndef _SPINLOCK_H
#define _SPINLOCK_H

typedef volatile unsigned int spinlock_t;

void acquire_spinlock(spinlock_t* spinlock);
int try_acquire_spinlock(spinlock_t* spinlock);
void release_spinlock(spinlock_t* spinlock);

void acquire_spinlock_irq(spinlock_t* spinlock, int *irqs);
void release_spinlock_irq(spinlock_t* spinlock, int irqs);

#endif