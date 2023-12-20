#include "ahci.h"
#include "ahci_defines.h"
#include "stdio.h"
#include "io.h"
#include "interrupts/handle/handler.h"
#include "stddef.h"
#include "atomic.h"
#include "mem/pmm.h"
#include "string.h"
#include "kstdlib.h"
#include "memory/kernel_vmm.h"
#include "mem/kheap.h"
#include "mem/iomem.h"
#include "drivers/storage/devices/storage_devices.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/interrupts.h"

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

void ahci_int_handler(interrupt_frame_t* frame, void* data) 
{
	ahci_controller_t* controller = (ahci_controller_t*) data;
	uint32 interrupt_pending = controller->hba_mem->is & controller->hba_mem->pi;

	for (int i = 0; i < 32; i++) {
		if (interrupt_pending & (1 << i)) {
			if (controller->ports[i].implemented) {
				ahci_port_interrupt(&controller->ports[i]);
			}
		}
	}

	// Очистка прерывания
	controller->hba_mem->is = interrupt_pending;
}

int ahci_controller_reset(ahci_controller_t* controller) 
{
	uint32_t pi = controller->hba_mem->pi;
	uint32_t caps = controller->hba_mem->cap & 
		(AHCI_CAPABILITY_MPSW | AHCI_CAPABILITY_SSS | AHCI_CAPABILITY_PMUL | AHCI_CAPABILITY_EMS | AHCI_CAPABILITY_EXS);

	// включение AHCI
	controller->hba_mem->ghc |= AHCI_CONTROLLER_GHC_AHCI_ENABLE;
	ahci_controller_flush_posted_writes(controller);
	
	// Перезапуск
	controller->hba_mem->ghc |= AHCI_CONTROLLER_GHC_RESET;
	ahci_controller_flush_posted_writes(controller);

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

	// включение AHCI
	controller->hba_mem->ghc |= AHCI_CONTROLLER_GHC_AHCI_ENABLE;
	ahci_controller_flush_posted_writes(controller);

	controller->hba_mem->pi = pi;
	controller->hba_mem->cap |= caps;
	ahci_controller_flush_posted_writes(controller);
    
    return 1;
}

void ahci_controller_enable_interrupts_ghc(ahci_controller_t* controller)
{
	controller->hba_mem->ghc |= AHCI_CONTROLLER_INTERRUPTS_ENABLE;
}

int ahci_device_probe(struct device *dev) {
	struct pci_device_info* device_desc = dev->pci_info;

	// Поиск подходящего PCI устройства
	ahci_controller_t* controller = (ahci_controller_t*)kmalloc(sizeof(ahci_controller_t));
	memset(controller, 0, sizeof(ahci_controller_t));

	controller->pci_device = device_desc;

	//Включить прерывания, DMA и MSA
	pci_set_command_reg(device_desc, pci_get_command_reg(device_desc) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
	pci_device_set_enable_interrupts(device_desc, 1);

	// Сохранение виртуального адреса на гллавную структуру контроллера
	controller->hba_mem = (HBA_MEMORY*) map_io_region(device_desc->BAR[5].address, device_desc->BAR[5].size);

	controller->version = controller->hba_mem->version;
	controller->capabilities = controller->hba_mem->cap;
	controller->capabilities_ext = controller->hba_mem->cap2;

	// Выключение прерываний
	controller->hba_mem->ghc &= ~AHCI_CONTROLLER_INTERRUPTS_ENABLE;
	ahci_controller_flush_posted_writes(controller);

	int reset = ahci_controller_reset(controller);
	if (!reset) {
		printf("AHCI controller reset failed !\n");
	}

	uint8_t irq = device_desc->interrupt_line;
	register_irq_handler(irq, ahci_int_handler, controller);

	// Получить информацию о портах. первый этап инициализации
	ahci_controller_probe_ports(controller);

	uint32_t interruptsPending;
	interruptsPending = controller->hba_mem->is;
	controller->hba_mem->is = interruptsPending;
	ahci_controller_flush_posted_writes(controller);
	ahci_controller_enable_interrupts_ghc(controller);
	ahci_controller_flush_posted_writes(controller);

	for (int i = 0; i < 32; i ++) {
		//Проверка, есть ли устройство
		if (controller->ports[i].implemented == 0) {
			continue;
		}

		ahci_port_init2(&controller->ports[i]);
	}

	for (int i = 0; i < 32; i ++) {
		//Проверка, есть ли устройство
		if (controller->ports[i].implemented == 0 || controller->ports[i].present == 0) {
			continue;
		}

		int dt = controller->ports[i].device_type;
		if (dt == AHCI_DEV_SATA)
		{
			printf("SATA drive found at port %i,", i);

			//Считать информацию об устройстве
			char* identity_buffer = pmm_alloc_page();
			memset(P2V(identity_buffer), 0, IDENTITY_BUFFER_SIZE);
			ahci_port_identity(&controller->ports[i], identity_buffer);

			drive_device_t* drive_header = new_drive_device_header();

			uint16_t type, capabilities;
			uint32_t cmd_sets;

			// Разобрать строку информации
			parse_identity_buffer(P2V(identity_buffer), 
									  &type,
									  &capabilities,
									  &cmd_sets,
									  &drive_header->sectors,
									  drive_header->model,
									  drive_header->serial);

			pmm_free_page(identity_buffer);

			drive_header->uses_lba48 = cmd_sets & (1 << 26);
			drive_header->bytes = (uint64_t)drive_header->sectors * 512;
			drive_header->ident = (drive_ident_t)&controller->ports[i];
				
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
			printf("SATAPI drive found at port %i,", i);
		}
		else if (dt == AHCI_DEV_SEMB)
		{
			printf("SEMB drive found at port %i,", i);
		}
		else if (dt == AHCI_DEV_PM)
		{
			printf("PM drive found at port %i,", i);
		}
		else
		{
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

	return 0;
}

struct pci_device_id ahci_ids[] = {
	{PCI_DEVICE_CLASS(0x1, 0x6, 0x01)},
	{0,}
};

struct device_driver_ops ahci_ops = {

    .probe = ahci_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_driver ahci_driver = {
	.dev_name = "Mass storage AHCI SATA Controller",
	.pci_device_ids = &ahci_ids,
	.ops = &ahci_ops
};

void ahci_init()
{
	register_pci_device_driver(&ahci_driver);

}
