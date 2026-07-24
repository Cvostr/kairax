#include "spinlock.h"
#include "proc/thread_scheduler.h"
#include "kairax/intctl.h"

void acquire_spinlock(spinlock_t* spinlock)
{
    while(!__sync_bool_compare_and_swap(spinlock, 0, 1))
	{
		asm volatile("pause");
	}
}

int try_acquire_spinlock(spinlock_t* spinlock)
{
	return __sync_bool_compare_and_swap(spinlock, 0, 1);
}

void release_spinlock(spinlock_t* spinlock)
{
    *spinlock = 0;
}

void acquire_spinlock_irq(spinlock_t* spinlock, int *irqs)
{
	*irqs = check_interrupts();
	if (*irqs == 1)
		disable_interrupts();

	acquire_spinlock(spinlock);
}

void release_spinlock_irq(spinlock_t* spinlock, int irqs)
{
	*spinlock = 0;
	if (irqs == 1)
		enable_interrupts();
}