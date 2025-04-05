#include "paging.h"
#include "mem/pmm.h"
#include "string.h"
#include "stdio.h"
#include "mem_layout.h"
#include "stddef.h"
#include "kstdlib.h"
#include "interrupts/apic.h"
#include "cpu/cpu_x64.h"

extern void x64_tlb_shootdown(void* addr);

page_table_t* new_page_table()
{
    page_table_t* table = (page_table_t*)pmm_alloc_page();
    if (table == NULL) {
        return NULL;
    }
    
    table = P2V(table);
    memset(table, 0, 4096);
    return table;
}

void* arch_new_vm_table()
{
    return new_page_table();
}

uint64_t x86_64_prot_2_flags(int protection) 
{
    uint64_t flags = PAGE_PRESENT;

    if (protection & PAGE_PROTECTION_WRITE_ENABLE) {
        flags |= PAGE_WRITABLE;
    }

    if (protection & PAGE_PROTECTION_USER) {
        flags |= PAGE_USER_ACCESSIBLE;
    }

    if ((protection & PAGE_PROTECTION_EXEC_ENABLE) == 0)
        flags |= PAGE_EXEC_DISABLE;

    return flags;
}

int arch_vm_map(void* arch_table, uint64_t vaddr, uint64_t physaddr, int prot)
{
    uint64_t flags = x86_64_prot_2_flags(prot);

    return map_page_mem(arch_table, vaddr, physaddr, flags);
}

int map_page_mem(page_table_t* root, virtual_addr_t virtual_addr, physical_addr_t physical_addr, uint64_t flags)
{
    //Вычисление индексов страниц
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    page_table_t *pdp_table;
    page_table_t *pd_table;
    page_table_t *pt_table;

    uint64_t baseFlags = PAGE_BASE_FLAGS;
    if ((flags & PAGE_USER_ACCESSIBLE) == PAGE_USER_ACCESSIBLE) {
        baseFlags |= PAGE_USER_ACCESSIBLE;
    }

    //Проверим, существует ли страница 4-го уровня
    if (!(root->entries[level4_index] & (PAGE_PRESENT))) {
        //Страница не существует
        //Выделить память под страницу
        pdp_table = new_page_table();
        if (pdp_table == NULL) {
            return ERR_NO_MEM;
        }

        //Записать страницу в родительское дерево
        root->entries[level4_index] = ((uint64_t)V2P(pdp_table) | baseFlags);  
    }

    pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    pdp_table = P2V(pdp_table);

    if(!(pdp_table->entries[level3_index] & (PAGE_PRESENT))){
        //Выделить память под страницу
        pd_table = new_page_table();
        if (pd_table == NULL) {
            return ERR_NO_MEM;
        }
        //Записать страницу в родительское дерево
        pdp_table->entries[level3_index] = ((uint64_t)V2P(pd_table) | baseFlags);  
    }

    pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    pd_table = P2V(pd_table);

    if(!(pd_table->entries[level2_index] & (PAGE_PRESENT))){
        //Выделить память под страницу
        pt_table = new_page_table();
        if (pt_table == NULL) {
            return ERR_NO_MEM;
        }
        //Записать страницу в родительское дерево
        pd_table->entries[level2_index] = ((uint64_t)V2P(pt_table) | baseFlags);  
    }

    pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    pt_table = P2V(pt_table);

    if((pt_table->entries[level1_index] & (PAGE_PRESENT)) == 0){
        pt_table->entries[level1_index] = ((uint64_t)physical_addr | flags);
        return 0;
    }

    return PAGING_ERROR_ALREADY_MAPPED;
}

void arch_vm_unmap(void* arch_table, uint64_t vaddr)
{
    uintptr_t phys = 0;
    int rc = unmap_page1(arch_table, vaddr, &phys);
    if (rc == 0) {
        pmm_free_page(phys);
    }
}

//Удалить виртуальную страницу
int unmap_page(page_table_t* root, uintptr_t virtual_addr)
{
    return unmap_page1(root, virtual_addr, NULL);
}

int unmap_page1(page_table_t* root, uintptr_t virtual_addr, uintptr_t* phys_addr)
{
    uint16_t level4_index = GET_4_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level3_index = GET_3_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level2_index = GET_2_LEVEL_PAGE_INDEX(virtual_addr);
    uint16_t level1_index = GET_1_LEVEL_PAGE_INDEX(virtual_addr);

    if (!(root->entries[level4_index] & PAGE_PRESENT)) {
        return ERR_NO_PAGE_PRESENT;
    }

    page_table_t* pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
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
        if (phys_addr) {
            *phys_addr = (uintptr_t) GET_PAGE_FRAME(pt_table->entries[level1_index]);
        }
        pt_table->entries[level1_index] = 0;
    }

    
    x64_tlb_shootdown(virtual_addr);

    // TODO: сделать сброс TLB более правильным - добавлять в очередь на tlb flush ядра адрес по процессу 
    lapic_send_ipi(0, IPI_DST_OTHERS, IPI_TYPE_FIXED, INTERRUPT_VEC_TLB);

    return 0;
}

uint64_t arch_vm_get_physical_addr(void* arch_table, uint64_t addr)
{
    return get_physical_address(arch_table, addr);
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

void destroy_page_table(table_entry_t* entries, int level, uint64_t mask)
{
    if (level > 0) {
        for (int i = 0; i < 512; i ++) {
            table_entry_t entry = entries[i];
            if ((entry & mask) == mask) {

                uintptr_t addr = GET_PAGE_FRAME(entry);
               
                destroy_page_table(P2V(addr), level - 1, mask);
            }
        }
    }    
    
    pmm_free_page(V2P(entries));
}

int set_page_flags(page_table_t* root, uintptr_t virtual_addr, uint64_t flags)
{
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
        return ERR_NO_PAGE_PRESENT;
    }

    pdp_table = GET_PAGE_FRAME(root->entries[level4_index]);
    pdp_table = P2V(pdp_table);

    if(!(pdp_table->entries[level3_index] & (PAGE_PRESENT))){
        return ERR_NO_PAGE_PRESENT;
    }

    pd_table = GET_PAGE_FRAME(pdp_table->entries[level3_index]);
    pd_table = P2V(pd_table);

    if(!(pd_table->entries[level2_index] & (PAGE_PRESENT))) {
        return ERR_NO_PAGE_PRESENT; 
    }

    pt_table = GET_PAGE_FRAME(pd_table->entries[level2_index]);
    pt_table = P2V(pt_table);

    uintptr_t paddr = GET_PAGE_FRAME(pt_table->entries[level1_index]);

    if((pt_table->entries[level1_index] & (PAGE_PRESENT)) == 0) {
        return ERR_NO_PAGE_PRESENT;
    }

    pt_table->entries[level1_index] = paddr | flags;

    x64_tlb_shootdown(virtual_addr);

    // TODO: остальные ядра

    return 0;
}

int page_table_mmap_fork(page_table_t* src, page_table_t* dest, struct mmap_range* area, int cow)
{
    uint64_t flags = x86_64_prot_2_flags(area->protection);

    /*if (cow) {
        // Для COW, только чтение, даже если право на запись было
        flags &= PAGE_WRITABLE;
    }

    printk("COW %i ", cow);

    for (uintptr_t address = area->base; address < area->base + area->length; address += PAGE_SIZE) 
    {
        physical_addr_t paddr = get_physical_address(src, address);
        map_page_mem(dest, address, paddr, flags);
    }*/

    for (uintptr_t address = area->base; address < align(area->base + area->length, PAGE_SIZE); address += PAGE_SIZE) 
    {
        physical_addr_t paddr = get_physical_address(src, address);
        char* newp = pmm_alloc_page();
        memcpy(P2V(newp), P2V(paddr), PAGE_SIZE);
        map_page_mem(dest, address, newp, flags);
    }

    return 0;
}

void arch_vm_protect(void* arch_table, uint64_t vaddr, int protection)
{
    uint64_t flags = x86_64_prot_2_flags(protection);
    page_table_t* root = (page_table_t*) arch_table;

    set_page_flags(root, vaddr, flags);
}

virtual_addr_t get_first_free_pages(page_table_t* root, uint64_t pages_count)
{
    return get_first_free_pages_from(0, root, pages_count);
}

virtual_addr_t get_first_free_pages_from(virtual_addr_t start, page_table_t* root, uint64_t pages_count)
{
    for (virtual_addr_t addr = start; addr < MAX_PAGES_4; addr += PAGE_SIZE) {
        if (!arch_vm_is_mapped(root, addr)) {
            int free = 0;
            for (virtual_addr_t saddr = addr; saddr < addr + pages_count * PAGE_SIZE; saddr += PAGE_SIZE) {
                if (!arch_vm_is_mapped(root, saddr)) {
                    free++;
                    if (free = pages_count) {
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