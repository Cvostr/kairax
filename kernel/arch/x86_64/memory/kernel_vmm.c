#include "kernel_vmm.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "stdio.h"
#include "string.h"
#include "kstdlib.h"

extern uintptr_t __KERNEL_START;
extern uintptr_t __KERNEL_END;
extern uintptr_t __KERNEL_VIRT_END;
extern uintptr_t __KERNEL_VIRT_LINK;

uintptr_t kernel_top_addr = 0;

page_table_t* root_pml4 = NULL;

page_table_t* get_kernel_pml4()
{
    return root_pml4;
}

uintptr_t vmm_get_virtual_address(uintptr_t addr)
{
    return P2V(addr);
}

void* vmm_get_physical_address(uintptr_t physical_addr) 
{
    return (void*)get_physical_address(root_pml4, physical_addr);
}

page_table_t* create_kernel_vm_map() 
{
    //Размер ядра
    size_t kernel_size = (uint64_t)&__KERNEL_VIRT_END - (uint64_t)&__KERNEL_VIRT_LINK;
    kernel_size = align(kernel_size, PAGE_SIZE);

    // Адрес, на котором кончается код ядра - к нему потом будут прилинкованы модули
    kernel_top_addr = P2K(kernel_size + 0x100000ULL);

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
	for (iter = 0; iter < kernel_size; iter += PAGE_SIZE) {
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
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, root_pml4, sizeof(page_table_t));
    return result;
}

void arch_destroy_vm_table(void* root)
{
    destroy_page_table(((page_table_t*)root)->entries, 4, PAGE_PRESENT | PAGE_USER_ACCESSIBLE);
}

uint64_t arch_expand_kernel_mem(uint64_t size)
{
    size = align(size, PAGE_SIZE);
    // Сохранить адрес для возврата
    uintptr_t temp = kernel_top_addr;
    uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;
    
    // todo : не кончилось ли 48 битное адресное пространство?

    for (uintptr_t iter = 0; iter < size; iter += PAGE_SIZE) {
		map_page_mem(root_pml4, temp + iter, pmm_alloc_page(), pageFlags);
        kernel_top_addr += PAGE_SIZE;
	}

    return temp;
}