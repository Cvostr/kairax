#ifndef _PAGING_H
#define _PAGING_H

#include "sync/spinlock.h"
#include "atomic.h"

#define PAGE_PROTECTION_WRITE_ENABLE    0b1
#define PAGE_PROTECTION_EXEC_ENABLE     0b10
#define PAGE_PROTECTION_USER            0b100

struct vm_table {
    void*       arch_table;
    spinlock_t  lock;
    atomic_t    refs;
};

struct vm_table* new_vm_table();

struct vm_table* clone_kernel_vm_table();

void free_vm_table(struct vm_table* table);

void vm_table_map(struct vm_table* table, uint64_t virtual_addr, uint64_t physical_addr, int protection);

void vm_table_unmap(struct vm_table* table, uint64_t virtual_addr);

void vm_memset(struct vm_table* table, uint64_t addr, int val, size_t size);

void vm_memcpy(struct vm_table* table, uint64_t dst, void* src, size_t size);

int vm_is_mapped(struct vm_table* table, uint64_t address);

// Общие архитектурно зависимые функции

void* arch_new_vm_table();

void* arch_clone_kernel_vm_table();

void arch_destroy_vm_table();

void arch_vm_map(void* arch_table, uint64_t vaddr, uint64_t physaddr, int prot);

void arch_vm_unmap(void* arch_table, uint64_t vaddr);

void arch_vm_memset(void* arch_table, uint64_t addr, int val, size_t size);

size_t arch_vm_memcpy(void* arch_table, uint64_t dst, void* src, size_t size);

int arch_vm_is_mapped(void* arch_table, uint64_t address);

#endif