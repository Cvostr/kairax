#ifndef MEM_LAYOUT_H
#define MEM_LAYOUT_H

#define MAX_ADDRESS             0xffffffffffffffff

#define USERSPACE_MMAP_ADDR     0x7fcc372d7000ULL
#define USERSPACE_MAX_ADDR      0x00007FFFFFFFFFFF

#define KERNEL_TEXT_OFFSET      0xFFFFFFFF80000000           //После этого адреса начинается область памяти ядра
#define PHYSICAL_MEM_MAP_OFFSET 0xFFFF888000000000           //После этого адреса начинается физическая память 
#define PHYSICAL_MEM_MAP_END    0xffffc87fffffffff

#define KHEAP_MAP_OFFSET        (0xffffc90000000000)
#define KHEAP_SIZE              512ULL * 1024 * 1024		     //512MB памяти кучи ядра

#include "types.h"

#define K2P(a) (void*)((uint64_t)(a) & ~KERNEL_TEXT_OFFSET)
#define P2K(a) (void*)((uint64_t)(a) | KERNEL_TEXT_OFFSET)

#define V2P(a) (void*)((uint64_t)(a) & ~PHYSICAL_MEM_MAP_OFFSET)
#define P2V(a) (void*)((uint64_t)(a) | PHYSICAL_MEM_MAP_OFFSET)

#endif