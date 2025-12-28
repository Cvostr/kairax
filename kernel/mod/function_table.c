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
#include "proc/thread.h"
#include "proc/thread_scheduler.h"
#include "net/eth.h"
#include "kairax/intctl.h"
#include "dev/type/net_device.h"
#include "net/net_buffer.h"
#include "dev/type/audio_endpoint.h"
#include "proc/tasklet.h"
#include "kairax/kstdlib.h"
#include "dev/device_man.h"
#include "drivers/char/input/keyboard.h"
#include "drivers/char/input/mouse.h"
#include "dev/type/drive_device.h"

#define KFUNCTION(x) {.name = #x, .func_ptr = x}

struct kernel_function functions[] = {
    KFUNCTION(printk),
    KFUNCTION(devfs_add_char_device),
    KFUNCTION(kmalloc),
    KFUNCTION(kfree),
    KFUNCTION(tm_to_epoch),
    KFUNCTION(register_pci_device_driver),
    KFUNCTION(pci_config_read16),
    KFUNCTION(pci_config_read32),
    KFUNCTION(pci_config_write16),
    KFUNCTION(pci_config_write32),
    KFUNCTION(pci_get_command_reg),
    KFUNCTION(pci_set_command_reg),
    KFUNCTION(pci_device_set_enable_interrupts),
    KFUNCTION(pci_device_get_irq_line),
    KFUNCTION(pci_device_is_msi_capable),
    KFUNCTION(pci_device_set_msi_vector),
    KFUNCTION(pci_device_is_msix_capable),
    KFUNCTION(pci_device_set_msix_vector),
    KFUNCTION(map_io_region),
    KFUNCTION(unmap_io_region),
    KFUNCTION(vmm_get_virtual_address),
    KFUNCTION(vmm_get_physical_address),
    KFUNCTION(pmm_alloc_page),
    KFUNCTION(pmm_alloc_pages),
    KFUNCTION(pmm_free_page),
    KFUNCTION(pmm_free_pages),
    KFUNCTION(pmm_alloc),
    KFUNCTION(register_irq_handler),
    KFUNCTION(alloc_irq),
    KFUNCTION(enable_interrupts),
    KFUNCTION(disable_interrupts),
    KFUNCTION(acquire_spinlock),
    KFUNCTION(release_spinlock),
    KFUNCTION(create_kthread),
    KFUNCTION(scheduler_add_thread),
    KFUNCTION(create_new_process),
    KFUNCTION(process_set_name),
    KFUNCTION(new_nic),
    KFUNCTION(register_nic),
    KFUNCTION(eth_handle_frame),
    KFUNCTION(new_net_buffer),
    KFUNCTION(net_buffer_acquire),
    KFUNCTION(net_buffer_free),
    KFUNCTION(new_audio_endpoint),
    KFUNCTION(register_audio_endpoint),
    KFUNCTION(audio_endpoint_gather_samples),
    KFUNCTION(tasklet_init),
    KFUNCTION(tasklet_schedule),
    KFUNCTION(new_device),
    KFUNCTION(device_set_parent),
    KFUNCTION(device_set_data),
    KFUNCTION(register_device),
    KFUNCTION(new_drive_device_info),
    KFUNCTION(keyboard_add_event),
    KFUNCTION(mouse_add_event),
    // usb
    KFUNCTION(new_usb_msg),
    KFUNCTION(usb_device_send_request),
    KFUNCTION(usb_device_send_async_request),
    KFUNCTION(register_usb_device_driver),
    KFUNCTION(usb_get_string),
    KFUNCTION(usb_device_configure_endpoint),
    KFUNCTION(usb_send_async_msg),
    KFUNCTION(usb_device_bulk_msg),
    KFUNCTION(usb_set_interface),
    // filesystems
    KFUNCTION(new_filesystem),
    KFUNCTION(filesystem_register),
    KFUNCTION(new_vfs_inode),
    KFUNCTION(new_vfs_dirent),
    // partition
    KFUNCTION(register_drive),
    KFUNCTION(partition_read),
    // kairax std
    KFUNCTION(memset),
    KFUNCTION(memcpy),
    KFUNCTION(strcpy),
    KFUNCTION(strncpy),
    KFUNCTION(strcmp),
    KFUNCTION(strlen),
    KFUNCTION(strcat),
    KFUNCTION(strncat),
    KFUNCTION(strchr),
    KFUNCTION(strrchr),
    KFUNCTION(align),
    KFUNCTION(tolower),
    KFUNCTION(itoa),
    // kairax list
    KFUNCTION(list_size),
    KFUNCTION(list_dequeue),
    KFUNCTION(list_add),
    KFUNCTION(list_pop)
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