#ifndef _PMM_H
#define _PMM_H

#include "types.h"

#define PAGE_SIZE 0x1000

typedef struct {
    uint64_t    physical_mem_total;

    uint64_t    kernel_base;
    uint64_t    kernel_size;
} pmm_params_t;

#define PMM_REGION_USABLE 1

struct pmm_region {
    uint64_t base;
    uint64_t length;
    int flags;
};

void init_pmm();

void pmm_add_region(uint64_t base, uint64_t length, int flags);
struct pmm_region* pmm_get_regions();
// Занять определенный регион памяти
void pmm_set_mem_region(uint64_t offset, uint64_t size, int mark_as_used);
void pmm_take_base_regions();

void* pmm_alloc_page();
void* pmm_alloc_pages(uint32_t pages);
void* pmm_alloc(size_t bytes, size_t* npages);

void pmm_free_page(void* addr);
void pmm_free_pages(void* addr, uint32_t pages);

uint64_t pmm_get_used_pages();
size_t pmm_get_physical_mem_size();
void pmm_set_params(pmm_params_t* params);

#endif