#ifndef KERNEL_VMM_H
#define KERNEL_VMM_H

#include "paging.h"

#define PHYSICAL_MEMORY_SIZE_OLD (1024ULL * 1024 * 512 * 64) //64 тб физической памяти

page_table_t* get_kernel_pml4();

page_table_t* create_kernel_vm_map();

void* vmm_get_physical_addr(physical_addr_t physical_addr);

#endif