#ifndef _PAGING_H
#define _PAGING_H

#include "sync/spinlock.h"
#include "atomic.h"

struct vm_table {
    void*       arch_table;
    spinlock_t  lock;
    atomic_t*   refs;
};

struct vm_table* new_vm_table();

void free_vm_table(struct vm_table* table);

void vm_table_map(struct vm_table* table, uint64_t virtual_addr, uint64_t physical_addr);

void vm_table_unmap(struct vm_table* table, uint64_t virtual_addr);

// Общие архитектурно зависимые функции

void* arch_new_vm_table();

void arch_detroy_vm_table();

void arch_vm_map(void* arch_table, uint64_t vaddr, uint64_t physaddr);

void arch_vm_unmap(void* arch_table, uint64_t vaddr);

#endif