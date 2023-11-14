#include "kernel_vmm.h"
#include "mem/pmm.h"
#include "stdio.h"
#include "string.h"
#include "kstdlib.h"

extern uintptr_t __KERNEL_START;
extern uintptr_t __KERNEL_END;
extern uintptr_t __KERNEL_VIRT_END;
extern uintptr_t __KERNEL_VIRT_LINK;

page_table_t* root_pml4 = NULL;

page_table_t* get_kernel_pml4()
{
    return root_pml4;
}

void* vmm_get_physical_addr(physical_addr_t physical_addr) 
{
    return (void*)get_physical_address(root_pml4, physical_addr);
}

page_table_t* create_kernel_vm_map() 
{
    //Размер ядра
    size_t kernel_size = (uint64_t)&__KERNEL_VIRT_END - (uint64_t)&__KERNEL_VIRT_LINK;
    kernel_size = align(kernel_size, PAGE_SIZE);

    uint64_t p4_page_flags = PAGE_PRESENT | PAGE_WRITABLE;
    uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;

	root_pml4 = new_page_table();

    // Выделить страницы 3-го уровня для старшей половины адресного пространства
    for (int i = 256; i < 512; i ++) {
        void* pdp_table = new_page_table();
        //Записать страницу в родительское дерево
        root_pml4->entries[i] = ((uint64_t) V2P(pdp_table) | p4_page_flags); 
    }

    uint64_t iter = 0;
    // маппинг текста ядра 
	for (iter = 0; iter <= kernel_size; iter += PAGE_SIZE) {
		map_page_mem(root_pml4, (virtual_addr_t)P2K(iter) + 0x100000ULL, 0x100000ULL + iter, pageFlags);
	}

    // маппинг регионов физической памяти
    struct pmm_region* reg = pmm_get_regions();
	while (reg->length != 0) {

        uint64_t base = align_down(reg->base, PAGE_SIZE);
        uint64_t len = align(reg->length, PAGE_SIZE);
		
        for (iter = base; iter <= base + len; iter += PAGE_SIZE) {
		    map_page_mem(root_pml4, (virtual_addr_t)P2V(iter), iter, pageFlags);
	    }

        reg++;
	}

    return root_pml4;
}

void vmm_use_kernel_vm()
{
    switch_pml4(V2P(root_pml4));
}

void* arch_clone_kernel_vm_table()
{
    page_table_t* result = new_page_table();
    memcpy(result, root_pml4, sizeof(page_table_t));
    return result;
}

void vmm_destroy_page_table(table_entry_t* entries, int level)
{
    if (level > 0) {
        for (int i = 0; i < 512; i ++) {
            table_entry_t entry = entries[i];
            if ((entry & PAGE_PRESENT) && (entry & PAGE_USER_ACCESSIBLE)) {

                uintptr_t addr = GET_PAGE_FRAME(entry);
               
                vmm_destroy_page_table(P2V(addr), level - 1);
            }
        }
    }    
    
    pmm_free_page(V2P(entries));
}

void arch_destroy_vm_table(void* root)
{
    vmm_destroy_page_table(((page_table_t*)root)->entries, 4);
}