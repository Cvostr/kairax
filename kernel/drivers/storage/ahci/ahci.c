#include "ahci.h"
#include "ahci_defines.h"
#include "stdio.h"
#include "io.h"
#include "interrupts/handle/handler.h"
#include "interrupts/pic.h"
#include "stddef.h"
#include "atomic.h"
#include "memory/pmm.h"
#include "string.h"
#include "memory/paging.h"

// Check device type

void ahci_controller_probe_ports(ahci_controller_t* controller){
	HBA_MEMORY* hba_mem = controller->hba_mem;
	// Search disk in implemented ports
	uint32_t pi = hba_mem->pi;
	uint32_t i = 0;
	while (i<32)
	{
		if (pi & 1)
		{
			HBA_PORT* hba_port = &hba_mem->ports[i]; 
			ahci_port_t* port = initialize_port(i, hba_port);
			/*int dt = check_type(&hba_mem->ports[i]);
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
			}*/
		}
 
		pi >>= 1;
		i ++;
	}
}

void ahci_int_handler(interrupt_frame_t* frame){
	
    printf("AHCI ");

	pic_eoi(11);
}

int ahci_controller_reset(ahci_controller_t* controller){
	controller->hba_mem->ghc = 1;

    full_memory_barrier();
    size_t retry = 0;

    while (1) {
        if (retry > 1000)
            return 0;
        if (!(controller->hba_mem->ghc & 1))
            break;
        io_delay(1000);
        retry++;
    }
    
    return 1;
}

void ahci_controller_enable_interrupts_ghc(ahci_controller_t* controller){
	controller->hba_mem->ghc |= (1 << 1);
}

void ahci_init(){
	//найти необходимое PCI устройство
	int pci_devices_count = get_pci_devices_count();
	for(int device_i = 0; device_i < pci_devices_count; device_i ++){
		pci_device_desc* device_desc = &get_pci_devices_descs()[device_i];

		if(device_desc->device_class == 0x1 && device_desc->device_subclass == 0x6 && device_desc->prog_if == 0x01){
			printf("SATA - AHCI controller found on bus: %i, device: %i func: %i IRQ: %i \n", 
			device_desc->bus,
			device_desc->device,
			device_desc->function,
			device_desc->interrupt_line);

			ahci_controller_t* controller = (ahci_controller_t*)alloc_page();
			memset(controller, 0, sizeof(ahci_controller_t));

			controller->pci_device = device_desc;
			controller->hba_mem = 0x2234560000;
			//Включить прерывания, DMA и MSA
			pci_set_command_reg(device_desc, pci_get_command_reg(device_desc) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
			pci_device_set_enable_interrupts(device_desc, 1);

			//создать виртуальную страницу с адресом BAR5 
			uint64_t pageFlags = PAGE_WRITABLE | PAGE_PRESENT | PAGE_UNCACHED;
			map_page_mem(get_kernel_pml4(), controller->hba_mem, device_desc->BAR[5].address, pageFlags);


			int reset = ahci_controller_reset(controller);
			if(!reset){
				printf("AHCI controller reset failed !\n");
			}
			
			printf("AHCI controller version %i\n", controller->hba_mem->version);

			register_interrupt_handler(0x20 + 11, ahci_int_handler);
    		pic_unmask(0x20 + 11);

			ahci_controller_enable_interrupts_ghc(controller);

			uint32_t capabilities = controller->hba_mem->cap;
    		uint32_t extended_capabilities = controller->hba_mem->cap2;

			printf("AHCI CAP %i %i\n", capabilities, extended_capabilities);

			ahci_controller_probe_ports(controller);
		}
	}
}
