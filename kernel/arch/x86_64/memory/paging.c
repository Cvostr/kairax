#include "paging.h"
#include "mem/pmm.h"
#include "string.h"
#include "stdio.h"
#include "hh_offset.h"
#include "stddef.h"

extern page_table_t p4_table;
page_table_t* root_pml4 = &p4_table;

page_table_t* get_kernel_pml4(){
    return root_pml4;
}

void set_kernel_pml4(page_table_t* pml4){
    root_pml4 = pml4;
}

page_table_t* new_page_table(){
    page_table_t* table = (page_table_t*)pmm_alloc_page();
    memset(table, 0, 4096);
    return table;
}

void map_page_mem(page_table_t* root, virtual_addr_t virtual_addr, physical_addr_t physical_addr, uint64_t flags){
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
        pdp_table = new_page_table();
        //Записать страницу в родительское дерево
        root->entries[level4_index] = ((uint64_t)pdp_table | flags);  
    }

    pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);

    if(!(pdp_table->entries[level3_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        //Выделить память под страницу
        pd_table = new_page_table();
        //Записать страницу в родительское дерево
        pdp_table->entries[level3_index] = ((uint64_t)pd_table | flags);  
    }

    pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);

    if(!(pd_table->entries[level2_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        //Выделить память под страницу
        pt_table = new_page_table();
        //Записать страницу в родительское дерево
        pd_table->entries[level2_index] = ((uint64_t)pt_table | flags);  
    }

    pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);

    if(!(pt_table->entries[level1_index] & (PAGE_PRESENT | PAGE_WRITABLE))){
        pt_table->entries[level1_index] = ((uint64_t)physical_addr | flags);
    }
}

void map_page(page_table_t* root, virtual_addr_t virtual_addr, uint64_t flags) {
    map_page_mem(root, virtual_addr, (physical_addr_t)pmm_alloc_page(), flags);
}

int unmap_page(page_table_t* root, uintptr_t virtual_addr){
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    if (!(root->entries[level4_index] & PAGE_PRESENT)) {
        return 1;
    }

    page_table_t * pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    if(!(pdp_table->entries[level3_index] & PAGE_PRESENT)){
        return 1;  
    }

    page_table_t* pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    if(!(pd_table->entries[level2_index] & PAGE_PRESENT)){
        return 1;
    }

    page_table_t* pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    if(!(pt_table->entries[level1_index] & PAGE_PRESENT)){
        return 1;
    }else{
        uintptr_t phys_addr = (uintptr_t)GET_PAGE_FRAME(pt_table->entries[level1_index]);
        free_page(phys_addr);
        pt_table->entries[level1_index] = 0;
    }

    return 0;
}

physical_addr_t get_physical_address(page_table_t* root, virtual_addr_t virtual_addr){
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    //printf("S %i %i %i %i %i\n", level4_index, level3_index, level2_index, level1_index, GET_PAGE_OFFSET(virtual_addr));
    if (!(root->entries[level4_index] & PAGE_PRESENT)) {
        return NULL;
    }

    page_table_t * pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    if(!(pdp_table->entries[level3_index] & PAGE_PRESENT)){
        return NULL;  
    }

    page_table_t* pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    if(!(pd_table->entries[level2_index] & PAGE_PRESENT)){
        return NULL;
    }

    page_table_t* pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    if(!(pt_table->entries[level1_index] & PAGE_PRESENT)){
        return NULL;
    }

    return GET_PAGE_FRAME(pt_table->entries[level1_index]) + GET_PAGE_OFFSET(virtual_addr);
}

int is_mapped(page_table_t* root, uintptr_t virtual_addr){
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    if (!(root->entries[level4_index] & PAGE_PRESENT)) {
        return FALSE;
    }

    page_table_t * pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    if(!(pdp_table->entries[level3_index] & PAGE_PRESENT)){
        return FALSE;  
    }

    page_table_t* pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    if(!(pd_table->entries[level2_index] & PAGE_PRESENT)){
        return FALSE;
    }

    page_table_t* pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);

    return pt_table->entries[level1_index] & PAGE_PRESENT;
}

virtual_addr_t get_first_free_pages(page_table_t* root, uint64_t pages_count){
    return get_first_free_pages_from(0, root, pages_count);
}

virtual_addr_t get_first_free_pages_from(virtual_addr_t start, page_table_t* root, uint64_t pages_count){
    for(virtual_addr_t addr = start; addr < MAX_PAGES_4; addr += PAGE_SIZE){
        if(!is_mapped(root, addr)){
            int free = 0;
            for(virtual_addr_t saddr = addr; saddr < addr + pages_count * PAGE_SIZE; saddr += PAGE_SIZE){
                if(!is_mapped(root, saddr)){
                    free++;
                    if(free = pages_count){
                        return addr;
                    }
                }else{
                    break;
                }
            }
        }
    }
    return NULL;
}

int copy_to_vm(page_table_t* root, virtual_addr_t dst, void* src, size_t size){
    int copied = 0;

    for(size_t i = 0; i < size; i ++){
        char* phys_addr = get_physical_address(root, dst + i);
        //printf("%i", phys_addr);
        if(phys_addr == NULL)
            return copied;
        *phys_addr = *(char*)src + i;
    }
    return copied;
}

void switch_pml4(page_table_t* pml4){
    write_cr3((uintptr_t)(pml4));
}