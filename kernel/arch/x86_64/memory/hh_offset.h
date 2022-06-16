#define KERNEL_OFFSET 0xFFFFFFFF80000000    //После этого адреса начинается область памяти ядра

#define PHYSICAL_MEMORY_SIZE (1024ULL * 1024 * 512 * 64) //64 тб физической памяти
#define KHEAP_SIZE            128ULL * 1024 * 1024		//128MB памяти кучи ядра


#include "stdint.h"

#define V2P(a) (void*)((uintptr_t)(a) & ~KERNEL_OFFSET)
#define P2V(a) ((uintptr_t)(a) | KERNEL_OFFSET)
