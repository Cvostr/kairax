#ifndef KX_KERNEL_H
#define KX_KERNEL_H

#include "stdint.h"
#include "stddef.h"

int KxLoadKernelModule(const unsigned char *image, size_t image_size);
int KxUnloadKernelModule(const char *name);

#endif