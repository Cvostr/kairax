#include "ahci.h"
#include "ahci_defines.h"
#include "dev/pci/pci.h"
#include "stdio.h"

void ahci_init(){
	//найти необходимое PCI устройство
	int pci_devices_count = get_pci_devices_count();
	for(int device_i = 0; device_i < pci_devices_count; device_i ++){
		pci_device_desc* device_desc = &get_pci_devices_descs()[device_i];

		if(device_desc->device_class == 0x1 && device_desc->device_subclass == 0x6 && device_desc->prog_if == 0x01){
			printf("SATA - AHCI controller found on bus: %i, device: %i func: %i command %i \n", 
			device_desc->pci_bus,
			device_desc->device,
			device_desc->function,
			device_desc->command);
		}
	}
}