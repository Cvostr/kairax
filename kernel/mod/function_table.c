#include "function_table.h"
#include "kairax/stdio.h"
#include "fs/devfs/devfs.h"
#include "kairax/string.h"
#include "dev/device_drivers.h"
#include "dev/bus/pci/pci.h"
#include "mem/kheap.h"
#include "mem/iomem.h"
#include "mem/vmm.h"
#include "mem/pmm.h"
#include "dev/interrupts.h"

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
    KFUNCTION(pci_config_write32),
    KFUNCTION(pci_get_command_reg),
    KFUNCTION(pci_set_command_reg),
    KFUNCTION(pci_device_set_enable_interrupts),
    KFUNCTION(pci_device_is_msi_capable),
    KFUNCTION(pci_device_set_msi_vector),
    KFUNCTION(map_io_region),
    KFUNCTION(unmap_io_region),
    KFUNCTION(vmm_get_virtual_address),
    KFUNCTION(vmm_get_physical_address),
    KFUNCTION(pmm_alloc_pages),
    KFUNCTION(pmm_free_pages),
    KFUNCTION(register_irq_handler),
    KFUNCTION(alloc_irq),
    KFUNCTION(acquire_spinlock),
    KFUNCTION(release_spinlock),
    // kairax std
    KFUNCTION(memset),
    KFUNCTION(memcpy),
    KFUNCTION(strcpy),
    KFUNCTION(strcmp),
    KFUNCTION(strlen),
    KFUNCTION(strcat)
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