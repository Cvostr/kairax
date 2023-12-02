#include "function_table.h"
#include "kairax/stdio.h"
#include "fs/devfs/devfs.h"
#include "string.h"

#define KFUNCTION(x) {.name = #x, .func_ptr = x}

struct kernel_function functions[] = {
    KFUNCTION(printk),
    KFUNCTION(devfs_add_char_device)
};

void* kfunctions_get_by_name(const char* name)
{
    size_t array_size = sizeof(functions) / sizeof(struct kernel_function);
    for (int i = 0; i < array_size; i ++) {
        struct kernel_function* kfunc = &functions[i];
        if (strcmp(kfunc->name, name) == 0) {
            return kfunc->func_ptr;
        }
    }

    return NULL;
}