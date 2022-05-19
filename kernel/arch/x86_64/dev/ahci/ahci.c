#include "ahci.h"
#include "ahci_defines.h"
#include "stdio.h"
#include "io.h"
#include "interrupts/handle/handler.h"
#include "interrupts/pic.h"

#include "memory/paging.h"

// Check device type
static int check_type(HBA_PORT *port)
{
	uint32_t ssts = port->ssts;
 
	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;
 
	printf("SSTS %i IPM %i DET %i SIG %i", ssts, ipm, det, port->sig);

	if (det != HBA_PORT_DET_PRESENT)	// Check drive status
		return AHCI_DEV_NULL;
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEV_NULL;

	switch (port->sig)
	{
	case SATA_SIG_ATAPI:
		return AHCI_DEV_SATAPI;
	case SATA_SIG_SEMB:
		return AHCI_DEV_SEMB;
	case SATA_SIG_PM:
		return AHCI_DEV_PM;
	default:
		return AHCI_DEV_SATA;
	}
}

void probe_port(HBA_MEMORY *abar)
{
	// Search disk in implemented ports
	uint32_t pi = abar->pi;
	int i = 0;
	while (i<32)
	{
		if (pi & 1)
		{
			int dt = check_type(&abar->ports[i]);
			printf("DT %i ", dt);
			if (dt == AHCI_DEV_SATA)
			{
				printf("SATA drive found at port %i\n", i);
			}
			else if (dt == AHCI_DEV_SATAPI)
			{
				printf("SATAPI drive found at port %i\n", i);
			}
			else if (dt == AHCI_DEV_SEMB)
			{
				printf("SEMB drive found at port %i\n", i);
			}
			else if (dt == AHCI_DEV_PM)
			{
				printf("PM drive found at port %i\n", i);
			}
			else
			{
				printf("No drive found at port %i\n", i);
			}
		}
 
		pi >>= 1;
		i ++;
	}
}

void ahci_int_handler(interrupt_frame_t* frame){
	
    printf("TIMER ");

	pic_eoi(11);
}

void ahci_init(){
	//найти необходимое PCI устройство
	int pci_devices_count = get_pci_devices_count();
	for(int device_i = 0; device_i < pci_devices_count; device_i ++){
		pci_device_desc* device_desc = &get_pci_devices_descs()[device_i];

		if(device_desc->device_class == 0x1 && device_desc->device_subclass == 0x6 && device_desc->prog_if == 0x01){
			printf("SATA - AHCI controller found on bus: %i, device: %i func: %i command %i IRQ: %i \n", 
			device_desc->bus,
			device_desc->device,
			device_desc->function,
			device_desc->command,
			device_desc->interrupt_line);

			register_interrupt_handler(0x20 + 11, ahci_int_handler);
    		pic_unmask(0x20 + 11);

			HBA_MEMORY* mem1 = (HBA_MEMORY*)device_desc->BAR[5].address;

			pci_enable_busmaster(device_desc);

			uint64_t pageFlags = PAGE_WRITABLE | PAGE_UNCACHED | PAGE_PRESENT;
			map_page(get_kernel_pml4(), mem1, pageFlags);

			mem1->ghc |= (uint32_t)1 << 31 | 1 << 1;
    		mem1->bohc |= 1 << 1;

			printf("%i %i %i\n ", device_desc->BAR[5].address, device_desc->BAR[5].size, device_desc->BAR[5].flags);
			probe_port(mem1);
		}


	}
}
