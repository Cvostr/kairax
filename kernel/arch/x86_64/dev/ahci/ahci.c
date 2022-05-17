#include "ahci.h"
#include "ahci_defines.h"
#include "stdio.h"
#include "io.h"
#include "memory/hh_offset.h"

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


void ahci_init(){
	//найти необходимое PCI устройство
	int pci_devices_count = get_pci_devices_count();
	for(int device_i = 0; device_i < pci_devices_count; device_i ++){
		pci_device_desc* device_desc = &get_pci_devices_descs()[device_i];

		if(device_desc->device_class == 0x1 && device_desc->device_subclass == 0x6 && device_desc->prog_if == 0x01){
			printf("SATA - AHCI controller found on bus: %i, device: %i func: %i command %i \n", 
			device_desc->bus,
			device_desc->device,
			device_desc->function,
			device_desc->command);

			/*HBA_MEMORY* mem1 = 0;
			uint8 reg_type = 5;
			uint8 locatable = 0;
			get_pci_bar_desc(device_desc->BAR5, &reg_type, &locatable, NULL, &mem1);

			mem1->ghc |= (uint32_t)1 << 31 | 1 << 1;
    		mem1->bohc |= 1 << 1;

			//HBA_MEMORY* mem = (HBA_MEMORY*)((uint64_t)(device_desc->BAR5));
			printf("%i %i %i\n ",reg_type, locatable, mem1);
			probe_port(mem1);*/
		}


	}
}
