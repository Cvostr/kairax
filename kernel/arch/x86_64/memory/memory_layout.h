#define KERNEL_OFFSET 0xFFFFFFFF80000000    //После этого адреса начинается область памяти ядра

#define PHYS_MEMORY_MAP_OFFSET  (0xffff880000000000)                                //Смещение для физической памяти
#define PHYSICAL_MEMORY_MAP_SIZE (0xffffc7ffffffffff - PHYS_MEMORY_MAP_OFFSET)      //64 тб физической памяти

#define KHEAP_MAP_OFFSET        (0xffffc90000000000)                                //Начало кучи ядра (kheap)
#define KHEAP_SIZE              (0xffffe8ffffffffff - KHEAP_MAP_OFFSET)		        //128MB памяти кучи ядра

#include "stdint.h"

#define V2P(a) (void*)((uintptr_t)(a) & ~KERNEL_OFFSET)
#define P2V(a) ((uintptr_t)(a) | KERNEL_OFFSET)
