#ifndef _ATOMIC_H
#define _ATOMIC_H

#include "types.h"

static inline void atomic_set(atomic_t *v, int i)
{
    v->counter = i;
}

static inline void atomic_inc(atomic_t *v)
{
	__sync_add_and_fetch(&v->counter, 1);
}

static inline void atomic_dec(atomic_t *v)
{
	__sync_sub_and_fetch(&v->counter, 1);
}

static inline int atomic_inc_and_get(atomic_t *v)
{
	return __sync_add_and_fetch(&v->counter, 1);
}

static inline int atomic_dec_and_test(atomic_t *v)
{
	return __sync_sub_and_fetch(&v->counter, 1) <= 0;
}

#define cmpxchg(ptr, oldval, newval) \
	__sync_val_compare_and_swap(ptr, oldval, newval)

static inline int atomic_cmpxchg(atomic_t *v, int oldval, int newval)
{
	return cmpxchg(&(v)->counter, oldval, newval);
}

enum MemoryOrder {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
};

static inline void atomic_signal_fence(int order) 
{
    return __atomic_signal_fence(order);
}

static inline void atomic_thread_fence(int order) 
{
    return __atomic_thread_fence(order);
}

static inline void full_memory_barrier() 
{
    atomic_signal_fence(memory_order_acq_rel);
    atomic_thread_fence(memory_order_acq_rel);
}

#endif