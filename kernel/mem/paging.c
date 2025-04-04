#include "paging.h"
#include "kheap.h"
#include "string.h"
#include "pmm.h"

struct vm_table* new_vm_table()
{
    struct vm_table* table = kmalloc(sizeof(struct vm_table));
    if (table == NULL) {
        return NULL;
    }

    memset(table, 0, sizeof(struct vm_table));

    return table; 
}

struct vm_table* clone_kernel_vm_table()
{
    struct vm_table* table = new_vm_table();
    if (table == NULL) {
        return NULL;
    }

    table->arch_table = arch_clone_kernel_vm_table();
    if (table->arch_table == NULL) {
        return NULL;
    }

    return table;
}

void free_vm_table(struct vm_table* table)
{
    if (atomic_dec_and_test(&table->refs)) {

        if (table->arch_table) {
            arch_destroy_vm_table(table->arch_table);
        }

        kfree(table);
    }
}

int vm_table_map(struct vm_table* table, uint64_t virtual_addr, uint64_t physical_addr, int protection)
{
    acquire_spinlock(&table->lock);

    int rc = arch_vm_map(table->arch_table, virtual_addr, physical_addr, protection);

    release_spinlock(&table->lock);

    return rc;
}

void vm_table_unmap_region(struct vm_table* table, uint64_t virtual_addr, uint64_t length)
{
    acquire_spinlock(&table->lock);

    for (uint64_t addr = virtual_addr; addr < virtual_addr + length; addr += PAGE_SIZE) {
        arch_vm_unmap(table->arch_table, addr);
    }

    release_spinlock(&table->lock);
}

void vm_table_protect_region(struct vm_table* table, uint64_t virtual_addr, uint64_t length, int protection)
{
    acquire_spinlock(&table->lock);

    for (uint64_t addr = virtual_addr; addr < virtual_addr + length; addr += PAGE_SIZE) {
        arch_vm_protect(table->arch_table, addr, protection);
    }

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
    acquire_spinlock(&table->lock);

    arch_vm_memset(table->arch_table, addr, val, size);

    release_spinlock(&table->lock);
}

void vm_memcpy(struct vm_table* table, uint64_t dst, void* src, size_t size)
{
    acquire_spinlock(&table->lock);

    arch_vm_memcpy(table->arch_table, dst, src, size);

    release_spinlock(&table->lock);
}

int vm_is_mapped(struct vm_table* table, uint64_t address)
{
    int mapped = 0;
    acquire_spinlock(&table->lock);

    mapped = arch_vm_is_mapped(table->arch_table, address);

    release_spinlock(&table->lock);

    return mapped;
}

uint64_t vm_get_physical_addr(struct vm_table* table, uint64_t addr)
{
    uint64_t paddr;
    acquire_spinlock(&table->lock);

    paddr = arch_vm_get_physical_addr(table->arch_table, addr);

    release_spinlock(&table->lock);

    return paddr;
}