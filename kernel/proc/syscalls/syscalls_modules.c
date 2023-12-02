#include "../syscalls.h"
#include "mod/module_loader.h"

int sys_load_module(void* module_image, size_t image_size)
{
    return module_load(module_image, image_size);
}