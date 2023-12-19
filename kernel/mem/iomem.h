#ifndef _IOMEM_H

#include "kairax/types.h"

// Отражение физического адреса в виртуальную память с запретом кэширования
uintptr_t map_io_region(uintptr_t base, size_t size);

int unmap_io_region(uintptr_t base, size_t size);

#endif