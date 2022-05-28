#include "kheap.h"
#include "stddef.h"
#include "memory/pmm.h"
#include "string.h"

kheap_t kheap;

int kheap_init(virtual_addr_t kheap_start){
    kheap.kheap_start = kheap_start;
    kheap.kheap_end = kheap_start;

    kheap.allocated_pages = 0;
    kheap.allocated_memory = 0;
}

virtual_addr_t kheap_expand(uint64_t size){
    uint64_t pages_count = size / PAGE_SIZE;
    if(size % PAGE_SIZE > 0)
        pages_count++;
    
    virtual_addr_t begin_addr = kheap.kheap_end;

    for(uint64_t page_i = 0; page_i < pages_count; page_i ++){
        physical_addr_t phys_page = alloc_page();
        virtual_addr_t  virt_addr = kheap.kheap_end + PAGE_SIZE * page_i;
        map_page_mem(get_kernel_pml4(), virt_addr, phys_page, PAGE_PRESENT | PAGE_WRITABLE);
        memset(virt_addr, 0, PAGE_SIZE);
    }

    /*kheap_item_t* new_item = (kheap_item_t*)begin_addr;
    new_item->free = 1;
    new_item->size = size - sizeof(kheap_item_t);
    new_item->prev = kheap.kheap_item_tail;

    kheap.kheap_item_tail->next = new_item;
    kheap.kheap_item_tail = new_item;*/

    return begin_addr;
}

kheap_item_t* kheap_next_free_region(uint64_t size){
    kheap_item_t* current_item = (kheap_item_t*)kheap.kheap_start;
    while(current_item != NULL){
        if(current_item->free && current_item->size >= size)
            return current_item;
        current_item = current_item->next;
    }
}

void* kheap_alloc(uint64_t size){
    return kheap_alloc_aligned(size, 1);
}

void* kheap_alloc_aligned(uint64_t size, uint64_t alignment){
    if(size == 0)
        return NULL;

    uint64_t alloc_size = ((size + (alignment - 1)) & ~(alignment)) + sizeof(kheap_item_t);

    
}