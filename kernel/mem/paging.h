#ifndef _PAGING_H
#define _PAGING_H

#include "sync/spinlock.h"
#include "atomic.h"

#define PAGE_PROTECTION_READ_ENABLE     0x01
#define PAGE_PROTECTION_WRITE_ENABLE    0x02
#define PAGE_PROTECTION_EXEC_ENABLE     0x04
#define PAGE_PROTECTION_USER            0x08

#define PAGING_ERROR_ALREADY_MAPPED       0x10
#define ERR_NO_PAGE_PRESENT               0x11

struct vm_table {
    void*       arch_table;
    spinlock_t  lock;
    atomic_t    refs;
};

struct vm_table* new_vm_table();

struct vm_table* clone_kernel_vm_table();

void free_vm_table(struct vm_table* table);

int vm_table_map(struct vm_table* table, uint64_t virtual_addr, uint64_t physical_addr, int protection);

void vm_table_unmap(struct vm_table* table, uint64_t virtual_addr);

void vm_table_unmap_region(struct vm_table* table, uint64_t virtual_addr, uint64_t length);

void vm_memset(struct vm_table* table, uint64_t addr, int val, size_t size);

void vm_memcpy(struct vm_table* table, uint64_t dst, void* src, size_t size);

int vm_is_mapped(struct vm_table* table, uint64_t address);

uint64_t vm_get_physical_addr(struct vm_table* table, uint64_t addr);

// Общие архитектурно зависимые функции

void* arch_new_vm_table();

void* arch_clone_kernel_vm_table();

void arch_destroy_vm_table();

int arch_vm_map(void* arch_table, uint64_t vaddr, uint64_t physaddr, int prot);

void arch_vm_unmap(void* arch_table, uint64_t vaddr);

void arch_vm_memset(void* arch_table, uint64_t addr, int val, size_t size);

size_t arch_vm_memcpy(void* arch_table, uint64_t dst, void* src, size_t size);

int arch_vm_is_mapped(void* arch_table, uint64_t address);

uint64_t arch_vm_get_physical_addr(void* arch_table, uint64_t addr);

// Расширить память текста ядра 
uint64_t arch_expand_kernel_mem(uint64_t size);

#endif