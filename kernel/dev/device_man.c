#include "device_man.h"
#include "list/list.h"
#include "device_drivers.h"
#include "string.h"
#include "mem/kheap.h"

list_t devices_list = {0,};
spinlock_t devices_lock = 0;

struct device* new_device()
{
    struct device* dev = kmalloc(sizeof(struct device));
	memset(dev, 0, sizeof(struct device));
	guid_generate(&dev->id);
    return dev;
}

int register_device(struct device* dev)
{
    dev->dev_state = DEVICE_STATE_UNINITIALIZED;

    acquire_spinlock(&devices_lock);
    list_add(&devices_list, dev);
    release_spinlock(&devices_lock);

    probe_device(dev);
}

struct device* get_device(int index)
{
    return get_device_with_type(DEVICE_TYPE_ANY, index);
}

struct device* get_device_with_type(int type, int index)
{
    acquire_spinlock(&devices_lock);
    struct list_node* current_node = devices_list.head;
    struct device* dev = NULL;
    struct device* result = NULL;
    int i = 0;

    while (current_node != NULL) {
        
        dev = (struct device*) current_node->element;
        // Переход на следующий элемент
        current_node = current_node->next;

        if (type != DEVICE_TYPE_ANY && dev->dev_type != type) {
            continue;
        }

        if (index == i) {
            result = dev;
            goto exit;
        }

        i++;
    }
exit:
    release_spinlock(&devices_lock);
    return result;
}

void probe_device(struct device* dev)
{
    if (dev->dev_state == DEVICE_STATE_UNINITIALIZED) {

        if (dev->dev_bus == DEVICE_BUS_PCI) {

            struct pci_device_info* pci_info = dev->pci_info;
            struct pci_device_id id = {
                .dev_class = pci_info->device_class,
                .dev_id = pci_info->device_id,
                .vendor_id = pci_info->vendor_id,
                .dev_subclass = pci_info->device_subclass,
                .dev_pif = pci_info->prog_if
            }; 

            struct pci_device_driver* drv = drivers_get_for_pci_device(&id);
            if (drv != NULL) {

                dev->dev_driver = drv;
                strcpy(dev->dev_name, drv->dev_name);

                if (drv->ops->probe) {
                    dev->dev_status_code = drv->ops->probe(dev);
                    if (dev->dev_status_code < 0) {
                        dev->dev_state = DEVICE_STATE_INIT_ERROR;
                    } else {
                        dev->dev_state = DEVICE_STATE_INITIALIZED;
                    }
                }
            }                
        }
    }
}

void probe_devices()
{
    struct list_node* current_node = devices_list.head;
    struct device* dev = NULL;

    while (current_node != NULL) {
        
        dev = (struct device*) current_node->element;

        probe_device(dev);
        
        // Переход на следующий элемент
        current_node = current_node->next;
    }
}