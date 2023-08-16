#include "paging.h"
#include "kheap.h"

struct vm_table* new_vm_table()
{
    struct vm_table* table = kmalloc(sizeof(vm_table));
    memset(table, 0, sizeof(struct vm_table));

    return result; 
}

void free_vm_table(struct vm_table* table)
{

}

void vm_table_map(struct vm_table* table, uint64_t virtual_addr, uint64_t physical_addr)
{

}

void vm_table_unmap(struct vm_table* table, uint64_t virtual_addr)
{

}