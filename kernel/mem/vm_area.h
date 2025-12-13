#ifndef VM_AREA_H
#define VM_AREA_H

#include "paging.h"
#include "kairax/atomic.h"
#include "fs/vfs/file.h"

#define MAP_SHARED	    0x01
#define MAP_PRIVATE     0x02
#define MAP_ANONYMOUS	0x20
#define MAP_STACK	    0x20000

struct mmap_range {
    struct vm_table *table;
    uint64_t        base;
    uint64_t        length;
    int             protection;
    int             flags;
    unsigned long   file_offset;
    struct file     *file;
    
    atomic_t        refs;
};

struct mmap_range* new_mmap_region();

void mmap_region_ref(struct mmap_range* region);
void mmap_region_unref(struct mmap_range* region);

int map_vm_region(struct mmap_range* region, uintptr_t phys_addr, uintptr_t offset, size_t size);
int unmap_vm_region(struct vm_table *table, struct mmap_range* region);

#endif