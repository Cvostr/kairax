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
#include "mem/kheap.h"

void ahci_controller_probe_ports(ahci_controller_t* controller){
	HBA_MEMORY* hba_mem = controller->hba_mem;
	// Поиск дисков в доступных портах
	uint32_t pi = hba_mem->pi;
	uint32_t i = 0;
	while (i<32)
	{
		if (pi & 1)
		{
			HBA_PORT* hba_port = &hba_mem->ports[i]; 
			ahci_port_t* port = initialize_port(&controller->ports[i], i, hba_port);
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

void ahci_controller_get_capabilities(ahci_controller_t* controller, uint32_t* capabilities, uint32_t* capabilities_ext){
	*capabilities = controller->hba_mem->cap;
	*capabilities_ext = controller->hba_mem->cap2;
}

uint32_t ahci_controller_get_version(ahci_controller_t* controller){
	return controller->hba_mem->version;
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

			ahci_controller_t* controller = (ahci_controller_t*)kmalloc(sizeof(ahci_controller_t));
			memset(controller, 0, sizeof(ahci_controller_t));

			controller->pci_device = device_desc;
			controller->hba_mem = device_desc->BAR[5].address;
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

			register_interrupt_handler(0x20 + 11, ahci_int_handler);
    		pic_unmask(0x20 + 11);

			ahci_controller_enable_interrupts_ghc(controller);

			ahci_controller_probe_ports(controller);

			for(int i = 0; i < 7; i ++){
				if(controller->ports[i].implemented == 0)
					continue;
				int dt = controller->ports[i].device_type;
				if (dt == AHCI_DEV_SATA)
				{
					printf("SATA drive found at port %i", i);

					char* wr = "HELLO WORLD";
					//ahci_port_write_lba48(&controller->ports[i], 0, 0, 1, V2P(wr));

					for(int si = 0; si < 2000; si++){
						uint8_t data[512];
						ahci_port_read_lba48(&controller->ports[i], si, 0, 1, V2P(data));
						for(int pi = 0; pi < 256; pi ++){
							printf("%c", data[pi]);
						}
						for(int ii = 0; ii < 1000000000; ii ++){
							asm volatile("nop");
						}
					}
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
					//printf("No drive found at port %i\n", i);
					continue;
				}
				switch (controller->ports[i].speed) {
					case 1:
						printf(" link speed 1.5Gb/s\n", __func__, i);
						break;
					case 2:
						printf(" link speed 3.0Gb/s\n", __func__, i);
						break;
					case 3:
						printf(" link speed 6.0Gb/s\n", __func__, i);
						break;
					default:
						printf(" link speed unrestricted\n", __func__, i);
						break;
				}
			}
		}
	}
}
