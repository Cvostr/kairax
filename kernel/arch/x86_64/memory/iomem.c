#include "paging.h"
#include "kernel_vmm.h"
#include "mem_layout.h"
#include "mem/pmm.h"
#include "kairax/kstdlib.h"

uintptr_t map_io_region(uintptr_t base, size_t size)
{
    size_t pages_num = align(size, PAGE_SIZE) / PAGE_SIZE;
    uint64_t pageFlags = PAGE_WRITABLE | PAGE_PRESENT | PAGE_UNCACHED;
    int res;

	for (int page_i = 0; page_i < pages_num; page_i ++) {
        
        uintptr_t vaddr = P2V(base) + page_i * PAGE_SIZE;

		res = map_page_mem(get_kernel_pml4(),
			vaddr,
			base + page_i * PAGE_SIZE,
			pageFlags);

        if (res == PAGING_ERROR_ALREADY_MAPPED)
        {
            set_page_flags(get_kernel_pml4(), vaddr, pageFlags);
        }

	}

    return P2V(base);
}

int unmap_io_region(uintptr_t base, size_t size)
{
    size_t pages_num = align(size, PAGE_SIZE) / PAGE_SIZE;

    for (int page_i = 0; page_i < pages_num; page_i ++) {

        unmap_page(get_kernel_pml4(), base + page_i * PAGE_SIZE);
	}

    return 0;
}