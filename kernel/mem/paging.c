#include "paging.h"
#include "kheap.h"
#include "string.h"

struct vm_table* new_vm_table()
{
    struct vm_table* table = kmalloc(sizeof(struct vm_table));
    memset(table, 0, sizeof(struct vm_table));

    table->arch_table = arch_new_vm_table();

    return table; 
}

void free_vm_table(struct vm_table* table)
{

}

void vm_table_map(struct vm_table* table, uint64_t virtual_addr, uint64_t physical_addr, int protection)
{
    acquire_spinlock(&table->lock);

    arch_vm_map(table->arch_table, virtual_addr, physical_addr, protection);

    release_spinlock(&table->lock);
}

void vm_table_unmap(struct vm_table* table, uint64_t virtual_addr)
{
    acquire_spinlock(&table->lock);

    arch_vm_unmap(table->arch_table, virtual_addr);

    release_spinlock(&table->lock);
}

void vm_memset(struct vm_table* table, uint64_t addr, int val, size_t size)
{
    arch_vm_memset(table->arch_table, addr, val, size);
}

void vm_memcpy(struct vm_table* table, uint64_t dst, void* src, size_t size)
{
    arch_vm_memcpy(table->arch_table, dst, src, size);
}