#ifndef KX_KERNEL_H
#define KX_KERNEL_H

#include <sys/cdefs.h>
#include "stdint.h"
#include "stddef.h"

__BEGIN_DECLS

int KxLoadKernelModule(const unsigned char *image, size_t image_size);
int KxUnloadKernelModule(const char *name);

__END_DECLS

#endif