#include "vm_area.h"
#include "mem/kheap.h"
#include "string.h"

struct mmap_range* new_mmap_region()
{
    struct mmap_range* region = kmalloc(sizeof(struct mmap_range));

    if (region == NULL)
    {
        return NULL;
    }

    memset(region, 0, sizeof(struct mmap_range));

    return region;
}

void mmap_region_ref(struct mmap_range* region)
{
    atomic_inc(&region->refs);
}

void mmap_region_unref(struct mmap_range* region)
{
    if (atomic_dec_and_test(&region->refs)) 
    {
        kfree(region);
    }
}