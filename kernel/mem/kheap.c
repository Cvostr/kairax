#include "kheap.h"
#include "stddef.h"
#include "memory/pmm.h"
#include "string.h"
#include "stdio.h"

kheap_t kheap;
#define MIN_ALLOC_SIZE sizeof(uint64_t)

static void heap_combine_forward(kheap_item_t* seg)
{
    if (seg->next == NULL || !seg->next->free)
        return;
    if (seg->next == kheap.tail)
        kheap.tail = seg;
    seg->size += seg->next->size + sizeof(kheap_item_t);
    seg->next = seg->next->next;
}

static void heap_combine_backward(kheap_item_t* seg)
{
    if (seg->prev != NULL && seg->prev->free)
        heap_combine_forward(seg->prev);
}

uint64_t align_4kb(uint64_t size){
    return size + (4096 - (size % 4096));
}

uint64_t get_pages_count(uint64_t size){
    return align_4kb(size) / PAGE_SIZE;
}

static uint64_t heap_expand(uint64_t size)
{
    uint64_t pages_paddr, pages_count, mapped_size, new_size;
    kheap_item_t* new;
    
    new = (kheap_item_t*) kheap.end_vaddr;
    
    new_size = align_4kb(size + sizeof(kheap_item_t));
    if (kheap.end_vaddr + new_size > kheap.ceil_vaddr)
        return 0;

    pages_count = get_pages_count(new_size);
    pages_paddr = (uint64_t)alloc_pages(pages_count);
    if (pages_paddr == 0)
        return 0;
    
    for(uintptr_t i = 0; i < pages_count; i ++){
        map_page_mem(get_kernel_pml4(), kheap.end_vaddr + i * PAGE_SIZE, pages_paddr + i * PAGE_SIZE, PAGE_PRESENT | PAGE_WRITABLE);
    }
    mapped_size = PAGE_SIZE * pages_count;
    kheap.end_vaddr += mapped_size;

    new->free = 1;
    new->size = new_size - sizeof(kheap_item_t);
    new->prev = kheap.tail;
    new->next = NULL;
    if (kheap.tail != NULL)
    {
        new->next = kheap.tail->next;
        kheap.tail->next = new;
    }
    kheap.tail = new;
    heap_combine_backward(new);

    return mapped_size;
}

static kheap_item_t* heap_next_free_segment(uint64_t size)
{
    kheap_item_t* current;
    
    current = kheap.head;
    while (current != NULL)
    {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    
    if (heap_expand(size) < size)
        return NULL;
    
    return heap_next_free_segment(size);
}

int kheap_init(uint64_t start_vaddr, uint64_t size){
    kheap_init_verbose(start_vaddr, start_vaddr + size, 8192);
}

int kheap_init_verbose(uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t initial_size)
{
    if (initial_size > ceil_vaddr - start_vaddr || initial_size < sizeof(kheap_item_t))
    {
        printf("Invalid initial size");
        return -1;
    }
    
    kheap.ceil_vaddr = ceil_vaddr;
    kheap.start_vaddr = start_vaddr;
    kheap.end_vaddr = start_vaddr;
    kheap.tail = NULL;
    if (heap_expand(initial_size) < initial_size)
    {
        printf("Failed to allocate initial memory");
        return -1;
    }
    kheap.head = kheap.tail;

    //printf("Kernel heap initialized at %i with size %i kB\n", kheap.start_vaddr, initial_size >> 10);

    return 0;
}

kheap_item_t* heap_allocate_memory(uint64_t size)
{
    kheap_item_t* seg;
    kheap_item_t* new;

    size = ((size < MIN_ALLOC_SIZE) ? MIN_ALLOC_SIZE : size);

    do {
        seg = heap_next_free_segment(size + sizeof(kheap_item_t));
        if (seg == NULL)
            return NULL;
    } while (seg->size < size);
    
    new = (kheap_item_t*) (((uint64_t)(seg + 1)) + size);
    new->free = 1;
    new->size = seg->size - size - sizeof(kheap_item_t);
    new->prev = seg;
    new->next = seg->next;
    if (new->next != NULL)
        new->next->prev = new;
    if (seg == kheap.tail)
        kheap.tail = new;

    seg->free = 0;
    seg->size = size;
    seg->next = new;

    return seg;
}

void heap_free_memory(kheap_item_t* seg)
{
    if (seg->next == NULL && seg != kheap.tail)
        seg = seg->prev;
    seg->free = 1;
    heap_combine_forward(seg);
    heap_combine_backward(seg);
}

void* kmalloc(uint64_t size){
    return heap_allocate_memory(size) + 1;
}

void kfree(void* mem){
    heap_free_memory(((kheap_item_t*)mem) - 1);
}

void print_seq(){
    kheap_item_t* current_item = kheap.head;
    while(current_item != NULL){
        printf("ITEM Addr : %i, Size : %i, Free : %i\n", current_item, current_item->size, current_item->free);
        current_item = current_item->next;
    }
}