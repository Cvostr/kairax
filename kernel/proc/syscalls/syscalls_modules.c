#include "../syscalls.h"
#include "mod/module_loader.h"
#include "mod/module_stor.h"

int sys_load_module(void* module_image, size_t image_size)
{
    return module_load(module_image, image_size);
}

int sys_unload_module(const char* module_name)
{
    return mstor_destroy_module(module_name);
}