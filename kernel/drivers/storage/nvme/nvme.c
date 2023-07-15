#include "nvme.h"
#include "stdio.h"
#include "string.h"
#include "mem/kheap.h"

void init_nvme()
{
    int pci_devices_count = get_pci_devices_count();
	for (int device_i = 0; device_i < pci_devices_count; device_i ++) {
		pci_device_desc* device_desc = &get_pci_devices_descs()[device_i];

		if(device_desc->device_class == 0x1 && device_desc->device_subclass == 0x8){
			printf("NVME controller found on bus: %i, device: %i func: %i IRQ: %i \n", 
			device_desc->bus,
			device_desc->device,
			device_desc->function,
			device_desc->interrupt_line);

			struct nvme_device* device = (struct nvme_device*)kmalloc(sizeof(struct nvme_device));
			memset(device, 0, sizeof(struct nvme_device));

			device->pci_device = device_desc;
			device->bar0 = (struct nvme_bar0*)device->pci_device->BAR[0].address;

			pci_set_command_reg(device_desc, pci_get_command_reg(device_desc) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
			
        }
    }
}