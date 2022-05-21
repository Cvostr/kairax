#ifndef _PMM_H
#define _PMM_H

#include "stdint.h"

#define PAGE_SIZE 0x1000

void init_pmm();

uintptr_t* alloc_page();

uintptr_t* alloc_pages(uint32_t pages);

void free_page(uint64_t addr);

void free_pages(uintptr_t* addr, uint32_t pages);

#endif