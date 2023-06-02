#include "kernel_vmm.h"
#include "mem/pmm.h"
#include "stdio.h"

extern uintptr_t __KERNEL_START;
extern uintptr_t __KERNEL_END;

page_table_t* root_pml4 = NULL;

page_table_t* get_kernel_pml4()
{
    return root_pml4;
}

page_table_t* new_page_table_bt(){
    page_table_t* table = (page_table_t*)pmm_alloc_page();
    table = P2K(table);
    memset(table, 0, 4096);
    return table;
}

void map_page_mem_bt(page_table_t* root, virtual_addr_t virtual_addr, physical_addr_t physical_addr, uint64_t flags){
    //Вычисление индексов страниц
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    page_table_t *pdp_table;
    page_table_t *pd_table;
    page_table_t *pt_table;

    //Проверим, существует ли страница 4-го уровня
    if (!(root->entries[level4_index] & (PAGE_PRESENT | PAGE_WRITABLE))) {
        //Страница не существует
        //Выделить память под страницу
        pdp_table = new_page_table_bt();
        //Записать страницу в родительское дерево
        root->entries[level4_index] = ((uint64_t)K2P(pdp_table) | flags);  
    }

    pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    pdp_table = P2K(pdp_table);

    if(!(pdp_table->entries[level3_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        //Выделить память под страницу
        pd_table = new_page_table_bt();
        //Записать страницу в родительское дерево
        pdp_table->entries[level3_index] = ((uint64_t)K2P(pd_table) | flags);  
    }

    pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    pd_table = P2K(pd_table);

    if(!(pd_table->entries[level2_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        //Выделить память под страницу
        pt_table = new_page_table_bt();
        //Записать страницу в родительское дерево
        pd_table->entries[level2_index] = ((uint64_t)K2P(pt_table) | flags);  
    }

    pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    pt_table = P2K(pt_table);

    if(!(pt_table->entries[level1_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        pt_table->entries[level1_index] = ((uint64_t)physical_addr | flags);
    }
}

page_table_t* create_kernel_vm_map() 
{
    //Максимальный адрес физической памяти
    size_t aligned_mem = pmm_get_physical_mem_max_addr();
    aligned_mem += (PAGE_SIZE - aligned_mem % PAGE_SIZE);

    //Размер ядра
    size_t kernel_size = (uint64_t)&__KERNEL_END - (uint64_t)&__KERNEL_START;
    kernel_size += (PAGE_SIZE - kernel_size % PAGE_SIZE);

    uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;
	page_table_t* new_pt = new_page_table_bt();

    // маппинг текста ядра 
	for (uintptr_t i = 0; i <= kernel_size * 2; i += PAGE_SIZE) {
		map_page_mem_bt(new_pt, P2K(i), i, pageFlags);
	}

    // маппинг физической памяти
	for (uintptr_t i = 0; i <= aligned_mem; i += PAGE_SIZE) {
		map_page_mem_bt(new_pt, P2V(i), i, pageFlags);
	}

    root_pml4 = new_pt;

    return new_pt;
}