#include "vm_area.h"
#include "mem/kheap.h"
#include "string.h"
#include "stdio.h"
#include "cpu/cpu_local.h"
#include "mem/pmm.h" 
#include "kairax/kstdlib.h"
#include "proc/process.h"

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
    if (region == NULL)
    {
        printk("warn: mmap_region_unref(NULL)\n");
        return;
    }

    if (atomic_dec_and_test(&region->refs)) 
    {
        if (region->file)
            file_close(region->file);
        kfree(region);
    }
}

int map_vm_region(struct mmap_range* region, uintptr_t phys_addr, uintptr_t offset, size_t size)
{
    int rc;
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    // Выровнять таблицу вниз
    uint64_t aligned_offset = align_down(offset, PAGE_SIZE);
    uint64_t aligned_size = align(offset, PAGE_SIZE);

    for (uint64_t i = aligned_offset; i < aligned_size; i += PAGE_SIZE)
    {
        if (region->flags & MAP_SHARED)
        {
            if ((rc = vm_table_map(process->vmemory_table, region->base + i, phys_addr, region->protection)) != 0)
                return rc;
        } 
        else
        {
            char* newp = pmm_alloc_page();
            memcpy(P2V(newp), P2V(phys_addr), PAGE_SIZE);

            if ((rc = vm_table_map(process->vmemory_table, region->base + i, newp, region->protection)) != 0)
                return rc;
        }

        phys_addr += PAGE_SIZE;
    }

    return 0;
}