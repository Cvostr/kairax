#include "kernel.h"
#include "ksyscalls.h"
#include "errno.h"

int KxLoadKernelModule(const unsigned char *image, size_t image_size)
{
    __set_errno( syscall_load_module(image, image_size));
}

int KxUnloadKernelModule(const char *name)
{
    __set_errno( syscall_unload_module(name));
}