#include "nvme.h"
#include "stdio.h"

void init_nvme(){
    int pci_devices_count = get_pci_devices_count();
	for(int device_i = 0; device_i < pci_devices_count; device_i ++){
		pci_device_desc* device_desc = &get_pci_devices_descs()[device_i];

		if(device_desc->device_class == 0x1 && device_desc->device_subclass == 0x8){
			printf("NVME controller found on bus: %i, device: %i func: %i IRQ: %i \n", 
			device_desc->bus,
			device_desc->device,
			device_desc->function,
			device_desc->interrupt_line);
        }
    }
}