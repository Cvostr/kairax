#include "kernel_vmm.h"
#include "mem/pmm.h"

page_table_t* root_pml4 = NULL;

page_table_t* get_kernel_pml4(){
    return root_pml4;
}

page_table_t* create_kernel_vm_map() {
    size_t aligned_mem = pmm_get_physical_mem_size();
    aligned_mem += (aligned_mem - aligned_mem % PAGE_SIZE);
    uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;
	page_table_t* new_pt = new_page_table();
    
	for(uintptr_t i = 0; i <= aligned_mem; i += PAGE_SIZE){
		map_page_mem(new_pt, P2V(i), i, pageFlags);
	}

    root_pml4 = new_pt;

    return new_pt;
}