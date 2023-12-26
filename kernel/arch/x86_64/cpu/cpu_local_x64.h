#ifndef _SCHED_CPU_H
#define _SCHED_CPU_H

#include "gdt.h"
#include "proc/thread.h"
#include "mem/paging.h"

struct cpu_local_x64 {
    // Указатели на стек пользователя и ядра. НЕ ПЕРЕМЕЩАТЬ!!!
    char* user_stack;
    char* kernel_stack;
    // НЕ ПЕРЕМЕЩАТЬ!!!

    gdt_entry_t* gdt;
    tss_t* tss;
    size_t gdt_size;
    int lapic_id;
    int id;

    struct vm_table* current_vm;
    struct thread* current_thread;
} PACKED;

static struct cpu_local_x64 __seg_gs * const this_core = 0;

static inline void cpu_set_kernel_stack(void* kstack_top)
{
    this_core->kernel_stack = kstack_top;
}

static inline void* get_user_stack_ptr()
{
    return this_core->user_stack;
}

static inline void set_user_stack_ptr(void* stack_ptr)
{
    this_core->user_stack = stack_ptr;
}

static inline void tss_set_rsp0(uintptr_t address)
{
    this_core->tss->rsp0 = address;
}

static inline void cpu_set_current_thread(struct thread* thr)
{
    this_core->current_thread = thr;
}

static inline struct thread* cpu_get_current_thread()
{
    return this_core->current_thread;
}

static inline void cpu_set_current_vm_table(struct vm_table* vmt)
{
    this_core->current_vm = vmt;
}

static inline struct vm_tabel* cpu_get_current_vm_table()
{
    return this_core->current_vm;
}

#endif
