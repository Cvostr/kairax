#include "device_man.h"
#include "list/list.h"
#include "device_drivers.h"

list_t* devices_list = NULL;

int register_device(struct device* dev)
{
    if (devices_list == NULL) {
        devices_list = create_list();
    }

    dev->dev_state = DEVICE_STATE_UNINITIALIZED;

    list_add(devices_list, dev);

    probe_device(dev);
}

struct device* get_device(int index)
{
    return list_get(devices_list, index);
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

                dev->dev_driver = dev;

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
    if (devices_list == NULL) {
        return;
    }

    struct list_node* current_node = devices_list->head;
    struct device* dev = NULL;

    for (size_t i = 0; i < devices_list->size; i++) {
        
        dev = (struct device*) current_node->element;

        probe_device(dev);
        
        // Переход на следующий элемент
        current_node = current_node->next;
    }
}