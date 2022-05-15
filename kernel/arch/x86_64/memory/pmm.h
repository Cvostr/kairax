#ifndef _PMM_H
#define _PMM_H

#include "stdint.h"

#define PAGE_SIZE 0x1000

void init_pmm();

uint64_t *alloc_page();

void free_page(uint64_t addr);

#endif