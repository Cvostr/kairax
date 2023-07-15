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

page_table_t* new_page_table_bt()
{
    page_table_t* table = (page_table_t*)pmm_alloc_page();
    table = P2K(table);
    memset(table, 0, 4096);
    return table;
}

void map_page_mem_bt(page_table_t* root, virtual_addr_t virtual_addr, physical_addr_t physical_addr, uint64_t flags)
{
    //Вычисление индексов страниц
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    page_table_t *pdp_table;
    page_table_t *pd_table;
    page_table_t *pt_table;

    pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    pdp_table = P2K(pdp_table);

    if(!(pdp_table->entries[level3_index] & (PAGE_PRESENT))){
        //Выделить память под страницу
        pd_table = new_page_table_bt();
        //Записать страницу в родительское дерево
        pdp_table->entries[level3_index] = ((uint64_t)K2P(pd_table) | flags);  
    }

    pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    pd_table = P2K(pd_table);

    if(!(pd_table->entries[level2_index] & (PAGE_PRESENT))){
        //Выделить память под страницу
        pt_table = new_page_table_bt();
        //Записать страницу в родительское дерево
        pd_table->entries[level2_index] = ((uint64_t)K2P(pt_table) | flags);  
    }

    pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    pt_table = P2K(pt_table);

    if(!(pt_table->entries[level1_index] & (PAGE_PRESENT))){
        pt_table->entries[level1_index] = ((uint64_t)physical_addr | flags);
    }
}

page_table_t* create_kernel_vm_map() 
{
    //Максимальный адрес физической памяти
    size_t aligned_mem = align(pmm_get_physical_mem_max_addr(), PAGE_SIZE);

    //Размер ядра
    size_t kernel_size = (uint64_t)&__KERNEL_VIRT_END - (uint64_t)&__KERNEL_VIRT_LINK;
    kernel_size = align(kernel_size, PAGE_SIZE);

    uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;
	root_pml4 = new_page_table_bt();

    for (int i = 255; i < 512; i ++) {
        void* pdp_table = new_page_table_bt();
        //Записать страницу в родительское дерево
        root_pml4->entries[i] = ((uint64_t)K2P(pdp_table) | pageFlags); 
    }

    uint64_t iter = 0;
    // маппинг текста ядра 
	for (iter = 0; iter <= kernel_size; iter += PAGE_SIZE) {
		map_page_mem_bt(root_pml4, (virtual_addr_t)P2K(iter) + 0x100000ULL, 0x100000ULL + iter, pageFlags);
	}

    // маппинг физической памяти
	for (iter = 0; iter <= aligned_mem; iter += PAGE_SIZE) {
		map_page_mem_bt(root_pml4, (virtual_addr_t)P2V(iter), iter, pageFlags);
	}

    return root_pml4;
}

page_table_t* vmm_clone_kernel_memory_map()
{
    page_table_t* result = new_page_table();
    memcpy(result, P2V(K2P(root_pml4)), sizeof(page_table_t));
    return result;
}

void vmm_destroy_page_table(table_entry_t* entries, int level)
{
    //printf("FREEING LEVEL %i\n", level);
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

void vmm_destroy_root_page_table(page_table_t* root)
{
    switch_pml4(K2P(root_pml4));
    vmm_destroy_page_table(root->entries, 4);
}