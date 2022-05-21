#ifndef _ATOMIC_H
#define _ATOMIC_H

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