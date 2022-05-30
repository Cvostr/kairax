#ifndef _KHEAP_H
#define _KHEAP_H

#include "stdint.h"
#include "memory/paging.h"

struct kheap_item
{
    struct kheap_item* next;
    struct kheap_item* prev;
    uint64_t size;
    uint64_t free : 1;
    uint64_t reserved : 63;
} PACKED;
typedef struct kheap_item kheap_item_t;

typedef struct {
    uint64_t start_vaddr;
    uint64_t end_vaddr;
    uint64_t ceil_vaddr;
    kheap_item_t* head;
    kheap_item_t* tail;
} kheap_t;

int kheap_init_verbose(uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t initial_size);

int kheap_init(uint64_t start_vaddr, uint64_t size);

kheap_item_t* heap_allocate_memory(uint64_t size);

void heap_free_memory(kheap_item_t* seg);

void* kmalloc(uint64_t size);

void kfree(void* mem);

void print_seq();

#endif