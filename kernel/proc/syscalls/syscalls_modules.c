#include "../syscalls.h"
#include "mod/module_loader.h"
#include "mod/module_stor.h"
#include "cpu/cpu_local_x64.h"
#include "kairax/errors.h"

int sys_load_module(void* module_image, size_t image_size)
{
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, module_image, image_size)

    if (process->euid != 0)
    {
        return -EPERM;
    }

    return module_load(module_image, image_size);
}

int sys_unload_module(const char* module_name)
{
    struct process* process = cpu_get_current_thread()->process;
    
    if (process->euid != 0)
    {
        return -EPERM;
    }

    return mstor_destroy_module(module_name);
}