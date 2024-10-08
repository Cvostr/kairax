#include "kheap.h"
#include "stddef.h"
#include "pmm.h"
#include "string.h"
#include "stdio.h"
#include "memory/kernel_vmm.h"
#include "sync/spinlock.h"
#include "kstdlib.h"

#define KHEAP_INIT_SIZE 4096000

kheap_t kheap;
spinlock_t kheap_lock;

uint64_t kheap_expand(uint64_t size)
{
    // Выравнивание до размера страницы (4096)
    size = align(size, PAGE_SIZE);

    // Вычисление количества страниц по 4 кб
    int pages_count = size / PAGE_SIZE;
    
    // Выделяем память и маппим страницы
    for (int i = 0; i < pages_count; i ++) {
        virtual_addr_t page_virtual = kheap.end_vaddr;
        physical_addr_t page_physical = pmm_alloc_page();
        map_page_mem(get_kernel_pml4(), page_virtual, page_physical, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
        memset((void*)page_virtual, 0, PAGE_SIZE);
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
    while (current_item != NULL) {

        if (current_item->free && current_item->size >= size) {
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
    acquire_spinlock(&kheap_lock);

    size = align(size, 8);

    //Размер данных и заголовка
    uint64_t total_size = size + sizeof(kheap_item_t);
    // Ищем подоходящий по размеру свободный блок
    kheap_item_t* current_item = get_suitable_item(total_size);
    void* result = NULL;

    if (current_item != NULL) {
        // Если размер блока гораздо больше - надо разделить
        if (current_item->size > (total_size + 8)) {
            // Адрес нового свободного блока, который будет создан
            kheap_item_t* new_item = (kheap_item_t*)((virtual_addr_t)(current_item + 1) + size);
            // Новый блок свободен
            new_item->free = 1;
            // Его размер с вычитанием длины заголовка
            new_item->size = current_item->size - total_size;
            // Предыдущим блоком нового блока будет текущий 
            new_item->prev = current_item;
            // Следующим блоком нового блока будет следующий для текущего
            new_item->next = current_item->next;

            if (new_item->next != NULL) {
                new_item->next->prev = new_item;
            }

            if (current_item == kheap.tail)
                kheap.tail = new_item;
            
            current_item->next = new_item;

            // Обновляем размер после деления
            current_item->size = size;
        }

        // Текущий блок теперь используется
        current_item->free = 0;

        result = current_item + 1;
    }

    release_spinlock(&kheap_lock);
    return result;
}

void combine_forward(kheap_item_t* item) 
{
    if(item == NULL)
        return;

    if(item->free == 0)
        return;

    // Указатель на следующий блок
    kheap_item_t* next_item = item->next;
    if (next_item != NULL) {
        if (next_item->free) {
            //Соединим
            kheap_item_t* next_next = next_item->next;

            // У следующего блока есть следующий, надо обновить связку
            if (next_next != NULL)
                next_next->prev = item;

            // Следующий блок - последний, надо обновить указатель на tail в главной структуре
            if (next_item == kheap.tail)
                kheap.tail = item;

            item->next = next_next;
            item->size += next_item->size + sizeof(kheap_item_t);
        }
    }
}

void kfree(void* mem)
{
    if (mem < KHEAP_MAP_OFFSET) {
        printk("KHEAP: PANIC: Invalid ADDR %i\n", mem);
    }

    acquire_spinlock(&kheap_lock);
    kheap_item_t* item = (kheap_item_t*)mem - 1;

    if(item->free == 0) {
        //Пометить как свободную
        item->free = 1;

        // Попытаться объеденить со следующей свободной ячейкой
        combine_forward(item);
        // Попытаться объеденить с предыдущей свободной ячейкой
        combine_forward(item->prev);
    }

    release_spinlock(&kheap_lock);
}

void* kheap_get_phys_address(void* mem)
{
    acquire_spinlock(&kheap_lock);
    void* result = get_physical_address(get_kernel_pml4(), (virtual_addr_t)mem);
    release_spinlock(&kheap_lock);

    return result;
}

kheap_item_t* kheap_get_head_item()
{
    return kheap.head;
}