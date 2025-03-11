#ifndef VM_AREA_H
#define VM_AREA_H

#include "atomic.h"

#define MAP_SHARED	    0x01
#define MAP_ANONYMOUS	0x20
#define MAP_STACK	    0x20000

struct mmap_range {
    uint64_t        base;
    uint64_t        length;
    int             protection;
    int             flags;
    
    atomic_t        refs;
};

struct mmap_range* new_mmap_region();

void mmap_region_ref(struct mmap_range* region);
void mmap_region_unref(struct mmap_range* region);

#endif