#ifndef _PMM_H
#define _PMM_H

#include "stdint.h"

#define PAGE_SIZE 0x1000

typedef struct {
    uint64_t    physical_mem_max_addr;
    uint64_t    physical_mem_total;

    uint64_t    kernel_base;
    uint64_t    kernel_size;
} pmm_params_t;

void init_pmm();

// Занять определенный регион памяти
void pmm_set_mem_region(uint64_t offset, uint64_t size);

void pmm_take_base_regions();

uintptr_t* pmm_alloc_page();

uintptr_t* pmm_alloc_pages(uint32_t pages);

void pmm_free_page(uint64_t addr);

void free_pages(uintptr_t* addr, uint32_t pages);

uint64_t pmm_get_used_pages();

size_t pmm_get_physical_mem_size();

void pmm_set_physical_mem_size(size_t size);

void pmm_set_physical_mem_max_addr(size_t size);

size_t pmm_get_physical_mem_max_addr();

#endif