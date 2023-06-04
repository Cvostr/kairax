#ifndef MEM_LAYOUT_H
#define MEM_LAYOUT_H

#define MAX_ADDRESS             0xffffffffffffffff
#define KERNEL_TEXT_OFFSET      0xFFFFFFFF80000000           //После этого адреса начинается область памяти ядра
#define PHYSICAL_MEM_MAP_OFFSET 0xFFFF888000000000           //После этого адреса начинается физическая память 

#define KHEAP_MAP_OFFSET        (0xffffc90000000000)
#define KHEAP_SIZE              512ULL * 1024 * 1024		     //128MB памяти кучи ядра

#include "stdint.h"

#define K2P(a) (void*)((uintptr_t)(a) & ~KERNEL_TEXT_OFFSET)
#define P2K(a) (void*)((uintptr_t)(a) | KERNEL_TEXT_OFFSET)

#define V2P(a) (void*)((uintptr_t)(a) & ~PHYSICAL_MEM_MAP_OFFSET)
#define P2V(a) (void*)((uintptr_t)(a) | PHYSICAL_MEM_MAP_OFFSET)

#endif