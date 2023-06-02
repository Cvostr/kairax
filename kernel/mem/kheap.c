#include "kheap.h"
#include "stddef.h"
#include "pmm.h"
#include "string.h"
#include "stdio.h"

#define KHEAP_INIT_SIZE 409600

kheap_t kheap;

uint64_t kheap_expand(uint64_t size)
{
    // Выравнивание до размера страницы (4096)
    if(size % PAGE_SIZE > 0){
        size += (PAGE_SIZE - (size % PAGE_SIZE));
    }

    int pages_count = size / PAGE_SIZE;

    physical_addr_t pages_physaddr = (physical_addr_t)pmm_alloc_pages(pages_count);
    if (pages_physaddr == 0)
        return 0; //Не получилось выделить физическую память
    
    for (int i = 0; i < pages_count; i ++) {
        virtual_addr_t page_virtual = kheap.end_vaddr;
        physical_addr_t page_physical = pages_physaddr + i * PAGE_SIZE;
        map_page_mem(get_kernel_pml4(), page_virtual, page_physical, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);

        kheap.end_vaddr += PAGE_SIZE;
    }

    return pages_count * PAGE_SIZE;
}

int kheap_init(uint64_t start_vaddr, uint64_t size)
{
    kheap.start_vaddr = start_vaddr;
    kheap.end_vaddr = kheap.start_vaddr;

    int rc = kheap_expand(KHEAP_INIT_SIZE);
    if (rc == 0) {
        return 0;
    }
    kheap.head = (kheap_item_t*)kheap.start_vaddr;
    kheap.head->free = 1;
    kheap.head->next = NULL;
    kheap.head->prev = NULL;
    kheap.head->size = KHEAP_INIT_SIZE - sizeof(kheap_item_t);

    kheap.tail = kheap.head;

    return 1;
}

kheap_item_t* get_suitable_item(uint64_t size)
{
    kheap_item_t* current_item = kheap.head;
    while(current_item != NULL) {

        if(current_item->free && current_item->size >= size) {
            return current_item;
        }
        else
            current_item = current_item->next;
    }

    if(current_item == NULL) {
        uint64_t allocated = kheap_expand(size);
        kheap.tail->size += allocated;
        return kheap.tail;
    }

    return NULL;
}

void* kmalloc(uint64_t size) 
{
    if(size % 8 > 0) {
        size += (8 - (size % 8));
    }
    //Размер данных и заголовка
    uint64_t total_size = size + sizeof(kheap_item_t);
    uint64_t reqd_size = total_size + sizeof(kheap_item_t);
    kheap_item_t* current_item = get_suitable_item(reqd_size);

    if(current_item != NULL) {
        uint64_t size_left = current_item->size - total_size;
        
        kheap_item_t* new_item = (kheap_item_t*)((virtual_addr_t)(current_item + 1) + size);

        new_item->free = 1;
        new_item->size = size_left - sizeof(kheap_item_t);

        new_item->prev = current_item;
        new_item->next = current_item->next;

        if(new_item->next != NULL) {
            new_item->next->prev = new_item;
        }

        if(current_item == kheap.tail)
            kheap.tail = new_item;
        
        current_item->next = new_item;
        current_item->free = 0;
        current_item->size = size;
    } else {
        return NULL;
    }

    return current_item + 1;
}

void combine_forward(kheap_item_t* item) 
{
    if(item == NULL)
        return;

    if(item->free == 0)
        return;

    // Указатель на следующий блок
    kheap_item_t* next_item = item->next;
    if(next_item != NULL) {
        if(next_item->free) {
            //Соединим
            kheap_item_t* next_next = next_item->next;

            // У следующего блока есть следующий, надо обновить связку
            if(next_next != NULL)
                next_next->prev = item;

            // Седующий блок - последний, надо обновить указатель на tail
            if(next_item == kheap.tail)
                kheap.tail = item;

            item->next = next_next;
            item->size += next_item->size + sizeof(kheap_item_t);
        }
    }
}

void kfree(void* mem)
{
    kheap_item_t* item = (kheap_item_t*)mem - 1;
    item->free = 1;

    kheap_item_t* prev = item->prev;

    combine_forward(item);
    //combine_forward(item->prev);
}

void* kheap_get_phys_address(void* mem)
{
    return get_physical_address(get_kernel_pml4(), (virtual_addr_t)mem);
}

kheap_item_t* kheap_get_head_item()
{
    return kheap.head;
}