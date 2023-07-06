#include "spinlock.h"
#include "proc/thread_scheduler.h"

void acquire_spinlock(spinlock_t* spinlock){
    while(!__sync_bool_compare_and_swap(spinlock, 0, 1))
	{
		asm volatile("pause");
	}
}

void acquire_mutex(spinlock_t* spinlock)
{
	while(!__sync_bool_compare_and_swap(spinlock, 0, 1))
	{
		scheduler_yield();
	}
}

void release_spinlock(spinlock_t* spinlock){
    *spinlock = 0;
}