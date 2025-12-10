#ifndef _VMM_H
#define _VMM_H

#include "kairax/types.h"

void* vmm_get_physical_address(uintptr_t vaddr);
uintptr_t vmm_get_virtual_address(uintptr_t addr);

#endif