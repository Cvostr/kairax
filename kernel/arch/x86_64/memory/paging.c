#include "paging.h"
#include "mem/pmm.h"
#include "string.h"
#include "stdio.h"
#include "mem_layout.h"
#include "stddef.h"

extern void write_cr3(void*);

page_table_t* new_page_table()
{
    page_table_t* table = (page_table_t*)pmm_alloc_page();
    table = P2V(table);
    memset(table, 0, 4096);
    return table;
}

void* arch_new_vm_table()
{
    return new_page_table();
}

void arch_vm_map(void* arch_table, uint64_t vaddr, uint64_t physaddr, int prot)
{
    uint64_t flags = PAGE_PRESENT;

    if (prot & PAGE_PROTECTION_WRITE_ENABLE)
        flags |= PAGE_WRITABLE;

    if (prot & PAGE_PROTECTION_USER)
        flags |= PAGE_USER_ACCESSIBLE;

    if ((prot & PAGE_PROTECTION_EXEC_ENABLE) == 0)
        flags |= PAGE_EXEC_DISABLE;

    map_page_mem(arch_table, vaddr, physaddr, flags);
}

void map_page_mem(page_table_t* root, virtual_addr_t virtual_addr, physical_addr_t physical_addr, uint64_t flags)
{
    //Вычисление индексов страниц
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    page_table_t *pdp_table;
    page_table_t *pd_table;
    page_table_t *pt_table;

    //Проверим, существует ли страница 4-го уровня
    if (!(root->entries[level4_index] & (PAGE_PRESENT))) {
        //Страница не существует
        //Выделить память под страницу
        pdp_table = new_page_table();
        //Записать страницу в родительское дерево
        root->entries[level4_index] = ((uint64_t)V2P(pdp_table) | flags);  
    }

    pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    pdp_table = P2V(pdp_table);

    if(!(pdp_table->entries[level3_index] & (PAGE_PRESENT))){
        //Выделить память под страницу
        pd_table = new_page_table();
        //Записать страницу в родительское дерево
        pdp_table->entries[level3_index] = ((uint64_t)V2P(pd_table) | flags);  
    }

    pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    pd_table = P2V(pd_table);

    if(!(pd_table->entries[level2_index] & (PAGE_PRESENT))){
        //Выделить память под страницу
        pt_table = new_page_table();
        //Записать страницу в родительское дерево
        pd_table->entries[level2_index] = ((uint64_t)V2P(pt_table) | flags);  
    }

    pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    pt_table = P2V(pt_table);

    if(!(pt_table->entries[level1_index] & (PAGE_PRESENT))){
        pt_table->entries[level1_index] = ((uint64_t)physical_addr | flags);
    }
}

void map_page(page_table_t* root, virtual_addr_t virtual_addr, uint64_t flags) {
    map_page_mem(root, virtual_addr, (physical_addr_t)pmm_alloc_page(), flags);
}

void arch_vm_unmap(void* arch_table, uint64_t vaddr)
{
    unmap_page(arch_table, vaddr);
}

int unmap_page(page_table_t* root, uintptr_t virtual_addr)
{
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    if (!(root->entries[level4_index] & PAGE_PRESENT)) {
        return ERR_NO_PAGE_PRESENT;
    }

    page_table_t * pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    pdp_table = P2V(pdp_table);
    if (!(pdp_table->entries[level3_index] & PAGE_PRESENT)) {
        return ERR_NO_PAGE_PRESENT;  
    }

    page_table_t* pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    pd_table = P2V(pd_table);
    if (!(pd_table->entries[level2_index] & PAGE_PRESENT)) {
        return ERR_NO_PAGE_PRESENT;
    }

    page_table_t* pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    pt_table = P2V(pt_table);
    if (!(pt_table->entries[level1_index] & PAGE_PRESENT)) {
        return ERR_NO_PAGE_PRESENT;
    } else {
        uintptr_t phys_addr = (uintptr_t)GET_PAGE_FRAME(pt_table->entries[level1_index]);
        pt_table->entries[level1_index] = 0;
    }

    return 0;
}

physical_addr_t get_physical_address(page_table_t* root, virtual_addr_t virtual_addr)
{
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    if (!(root->entries[level4_index] & PAGE_PRESENT)) {
        return NULL;
    }

    page_table_t * pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    pdp_table = P2V(pdp_table);
    if(!(pdp_table->entries[level3_index] & PAGE_PRESENT)){
        return NULL;  
    }

    page_table_t* pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    pd_table = P2V(pd_table);
    if(!(pd_table->entries[level2_index] & PAGE_PRESENT)){
        return NULL;
    }

    page_table_t* pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    pt_table = P2V(pt_table);
    if(!(pt_table->entries[level1_index] & PAGE_PRESENT)){
        return NULL;
    }

    return (physical_addr_t)GET_PAGE_FRAME(pt_table->entries[level1_index]) + GET_PAGE_OFFSET(virtual_addr);
}

int arch_vm_is_mapped(void* arch_table, uint64_t address)
{
    page_table_t* root = (page_table_t*) arch_table;
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(address);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(address);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(address);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(address);

    if (!(root->entries[level4_index] & PAGE_PRESENT)) {
        return 0;
    }

    page_table_t * pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    pdp_table = P2V(pdp_table);
    if(!(pdp_table->entries[level3_index] & PAGE_PRESENT)){
        return 0;  
    }

    page_table_t* pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    pd_table = P2V(pd_table);
    if(!(pd_table->entries[level2_index] & PAGE_PRESENT)){
        return 0;
    }

    page_table_t* pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    pt_table = P2V(pt_table);

    return pt_table->entries[level1_index] & PAGE_PRESENT;
}

int set_page_flags(page_table_t* root, uintptr_t virtual_addr, uint64_t flags)
{
    int rc = 0;
    physical_addr_t phys_addr = get_physical_address(root, virtual_addr);
    unmap_page(root, virtual_addr);
	map_page_mem(root, virtual_addr, phys_addr, flags);

    return 0;
}

virtual_addr_t get_first_free_pages(page_table_t* root, uint64_t pages_count)
{
    return get_first_free_pages_from(0, root, pages_count);
}

virtual_addr_t get_first_free_pages_from(virtual_addr_t start, page_table_t* root, uint64_t pages_count){
    for(virtual_addr_t addr = start; addr < MAX_PAGES_4; addr += PAGE_SIZE){
        if(!arch_vm_is_mapped(root, addr)){
            int free = 0;
            for(virtual_addr_t saddr = addr; saddr < addr + pages_count * PAGE_SIZE; saddr += PAGE_SIZE){
                if(!arch_vm_is_mapped(root, saddr)){
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

size_t arch_vm_memcpy(void* arch_table, uint64_t dst, void* src, size_t size)
{
    size_t copied = 0;

    for (size_t i = 0; i < size; i ++) {
        char* phys_addr = (char*)get_physical_address((page_table_t*) arch_table, dst + i);
        
        if (phys_addr == NULL) {
            return copied;
        }

        phys_addr = P2V(phys_addr);
        *phys_addr = *((char*)src + i);
        copied ++;
    }
    
    return copied;
}

void arch_vm_memset(void* arch_table, uint64_t addr, int val, size_t size)
{
    for (virtual_addr_t i = addr; i < addr + size; i += 1) {
        arch_vm_memcpy(arch_table, i, &val, 1);
    }
}

void switch_pml4(page_table_t* pml4)
{
    write_cr3(pml4);
}