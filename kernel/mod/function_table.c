#include "function_table.h"
#include "kairax/stdio.h"
#include "fs/devfs/devfs.h"
#include "string.h"
#include "dev/device_drivers.h"
#include "dev/bus/pci/pci.h"
#include "mem/kheap.h"

#define KFUNCTION(x) {.name = #x, .func_ptr = x}

struct kernel_function functions[] = {
    KFUNCTION(printk),
    KFUNCTION(devfs_add_char_device),
    KFUNCTION(kmalloc),
    KFUNCTION(kfree),
    KFUNCTION(register_pci_device_driver),
    KFUNCTION(pci_config_read16),
    KFUNCTION(pci_config_read32),
    KFUNCTION(pci_config_write16),
    KFUNCTION(pci_config_write32)
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