#include "ahci.h"
#include "ahci_defines.h"
#include "stdio.h"
#include "io.h"
#include "interrupts/handle/handler.h"
#include "interrupts/pic.h"
#include "stddef.h"
#include "atomic.h"
#include "mem/pmm.h"
#include "string.h"
#include "stdlib.h"
#include "memory/kernel_vmm.h"
#include "mem/kheap.h"
#include "drivers/storage/devices/storage_devices.h"

void ahci_controller_probe_ports(ahci_controller_t* controller){
	// Виртуальный адрес HBA_MEMORY
	HBA_MEMORY* hba_mem = controller->hba_mem;
	// Поиск дисков в доступных портах
	uint32_t pi = hba_mem->pi;
	uint32_t i = 0;
	while (i < 32)
	{
		if (pi & 1)
		{
			HBA_PORT* hba_port = (HBA_PORT*)P2V(&hba_mem->ports[i]); 
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

		// Поиск подходящего PCI устройства
		if(device_desc->device_class == 0x1 && device_desc->device_subclass == 0x6 && device_desc->prog_if == 0x01){
			ahci_controller_t* controller = (ahci_controller_t*)kmalloc(sizeof(ahci_controller_t));
			memset(controller, 0, sizeof(ahci_controller_t));

			controller->pci_device = device_desc;
			controller->hba_mem = (HBA_MEMORY*)device_desc->BAR[5].address;
			//Включить прерывания, DMA и MSA
			pci_set_command_reg(device_desc, pci_get_command_reg(device_desc) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
			pci_device_set_enable_interrupts(device_desc, 1);

			// Данная страница уже замаплена, но без PAGE_UNCACHED
			// Перезамаппим её
			uint64_t pageFlags = PAGE_WRITABLE | PAGE_PRESENT | PAGE_UNCACHED;
			unmap_page(get_kernel_pml4(), (uintptr_t)P2V(device_desc->BAR[5].address));
			map_page_mem(get_kernel_pml4(), P2V(device_desc->BAR[5].address), device_desc->BAR[5].address, pageFlags);

			//!!!!
			controller->hba_mem = (HBA_MEMORY*)P2V(controller->hba_mem);

			int reset = ahci_controller_reset(controller);
			if(!reset){
				printf("AHCI controller reset failed !\n");
			}

			register_interrupt_handler(0x20 + 11, ahci_int_handler);
    		pic_unmask(0x20 + 11);

			ahci_controller_enable_interrupts_ghc(controller);

			ahci_controller_probe_ports(controller);

			for (int i = 0; i < 32; i ++) {
				//Проверка, есть ли устройство
				if(controller->ports[i].implemented == 0) {
					continue;
				}

				int dt = controller->ports[i].device_type;
				if (dt == AHCI_DEV_SATA)
				{
					printf("SATA drive found at port %i, \n", i);

					//Считать информацию об устройстве
					char* identity_buffer = kmalloc(IDENTITY_BUFFER_SIZE);
					memset(identity_buffer, 0, IDENTITY_BUFFER_SIZE);
					ahci_port_identity(&controller->ports[i], vmm_get_physical_addr(identity_buffer));

					drive_device_t* drive_header = new_drive_device_header();

					uint16_t type, capabilities;
					uint32_t cmd_sets;

					parse_identity_buffer(identity_buffer, 
										  &type,
										  &capabilities,
										  &cmd_sets,
										  &drive_header->sectors,
										  drive_header->model,
										  drive_header->serial);

					kfree(identity_buffer);

					drive_header->uses_lba48 = cmd_sets & (1 << 26);
					drive_header->bytes = (uint64_t)drive_header->sectors * 512;
					drive_header->ident = &controller->ports[i];
					
					strcpy(drive_header->name, "ahci");
					strcat(drive_header->name, itoa(0, 10));
					strcat(drive_header->name, "n");
					strcat(drive_header->name, itoa(i, 10));
					
					drive_header->write = ahci_port_write_lba;
					drive_header->read = ahci_port_read_lba;
					add_storage_device(drive_header);
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
				/*switch (controller->ports[i].speed) {
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
				}*/
			}
		}
	}
}
