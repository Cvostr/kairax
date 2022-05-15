#include "paging.h"
#include "pmm.h"
#include "string.h"
#include "stdio.h"
#include "hh_offset.h"

extern page_table_t p4_table;

page_table_t* get_kernel_pml4(){
    return (&p4_table);
}

void map_page(page_table_t* root, uintptr_t virtual_addr, uint64_t flags) {
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
        pdp_table = (page_table_t*)alloc_page();
        memset(pdp_table, 0, 4096);
        //Записать страницу в родительское дерево
        root->entries[level4_index] = ((uint64_t)pdp_table | flags);  
    }

    pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);

    if(!(pdp_table->entries[level3_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        //Выделить память под страницу
        pd_table = (page_table_t*)alloc_page();
        memset(pd_table, 0, 4096);

        pdp_table->entries[level3_index] = ((uint64_t)pd_table | flags);  
    }

    pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);

    if(!(pd_table->entries[level2_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        //Выделить память под страницу
        pt_table = (page_table_t*)alloc_page();
        memset(pt_table, 0, 4096);

        pd_table->entries[level2_index] = ((uint64_t)pt_table | flags);  
    }

    pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);

    if(!(pt_table->entries[level1_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        uint64_t* freepage = (uint64_t*)alloc_page();
        pt_table->entries[level1_index] = ((uint64_t)freepage | flags);
    }
}

void unmap_page(page_table_t* root, uintptr_t virtual_addr, uint64_t flags){
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);
}

physical_addr_t get_physical_address(page_table_t* root, virtual_addr_t virtual_addr){
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t offset = GET_PAGE_OFFSET(virtual_addr);

    //printf("%i %i %i %i", level4_index, level3_index, level2_index, level1_index);

    //Проверим, существует ли страница 4-го уровня
    if (!(root->entries[level4_index] & (PAGE_PRESENT | PAGE_WRITABLE))) {
        return 0x0;
    }

    page_table_t * pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    if(!(pdp_table->entries[level3_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        return 0x0;  
    }

    page_table_t* pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    if(!(pd_table->entries[level2_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        return 0x0;
    }

    page_table_t* pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    if(!(pt_table->entries[level1_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        return 0x0;
    }

    return pt_table->entries[level1_index] + offset;
}

int is_mapped(page_table_t* root, uintptr_t virtual_addr){
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    //Проверим, существует ли страница 4-го уровня
    if (!(root->entries[level4_index] & (PAGE_PRESENT | PAGE_WRITABLE))) {
        return FALSE;
    }

    page_table_t * pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    if(!(pdp_table->entries[level3_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        return FALSE;  
    }

    page_table_t* pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    if(!(pd_table->entries[level2_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        return FALSE;
    }

    page_table_t* pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    if(!(pt_table->entries[level1_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        return FALSE;
    }

    return pt_table->entries[level1_index] & PAGE_PRESENT;
}

void switch_pml4(page_table_t* pml4){
    write_cr3((uintptr_t)(pml4));
}