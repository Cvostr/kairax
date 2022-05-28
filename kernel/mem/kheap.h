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
    virtual_addr_t kheap_start;
    virtual_addr_t kheap_end;

    kheap_item_t*  kheap_item_head;
    kheap_item_t*  kheap_item_tail;

    uint64_t allocated_pages;
    uint64_t allocated_memory;
} kheap_t;

int kheap_init(virtual_addr_t kheap_start);

void* kheap_alloc(uint64_t size);

void* kheap_alloc_aligned(uint64_t size, uint64_t alignment);

int kheap_free(void* address);

#endif