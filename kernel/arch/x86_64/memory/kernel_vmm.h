#ifndef KERNEL_VMM_H
#define KERNEL_VMM_H

#include "paging.h"

page_table_t* create_kernel_vm_map();

page_table_t* get_kernel_pml4();

void vmm_use_kernel_vm();

page_table_t* vmm_clone_kernel_memory_map();

#endif