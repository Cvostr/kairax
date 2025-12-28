#include "ahci.h"
#include "ahci_defines.h"
#include "kairax/stdio.h"
#include "dev/interrupts.h"
#include "stddef.h"
#include "kairax/atomic.h"
#include "module.h"
#include "functions.h"
#include "kairax/string.h"
#include "kairax/kstdlib.h"
#include "mem/iomem.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/interrupts.h"
#include "dev/device_man.h"

char* ahci_link_speed_str(int speed)
{
	switch (speed) {
		case 1:
			return "1.5Gb/s";
			break;
		case 2:
			return "3.0Gb/s";
			break;
		case 3:
			return "6.0Gb/s";
			break;
		default:
			return "unrestricted";
			break;
	}

	return NULL;
}

void ahci_form_disk_name(char* dst, int controller_i, int disk_i)
{
	strcpy(dst, "ahci");
	strcat(dst, itoa(controller_i, 10));
	strcat(dst, "n");
	strcat(dst, itoa(disk_i, 10));
}

void ahci_controller_probe_ports(ahci_controller_t* controller)
{
	// Виртуальный адрес HBA_MEMORY
	HBA_MEMORY* hba_mem = controller->hba_mem;
	// Поиск дисков в доступных портах
	uint32_t pi = hba_mem->pi;
	uint32_t i = 0;
	while (i < 32)
	{
		if (pi & 1)
		{
			HBA_PORT* hba_port = &hba_mem->ports[i]; 
			initialize_port(&controller->ports[i], i, hba_port);
		}
 
		pi >>= 1;
		i ++;
	}
}

void ahci_irq_handler(void* frame, void* data) 
{
	ahci_controller_t* controller = (ahci_controller_t*) data;
	uint32 interrupt_pending = controller->hba_mem->is & controller->hba_mem->pi;

	if (interrupt_pending == 0)
		return;

	//printk("AHCI Interrupt %i\n", interrupt_pending);
	for (int i = 0; i < 32; i++) {
		if (interrupt_pending & (1 << i)) {
			if (controller->ports[i].implemented) {
				ahci_port_interrupt(&controller->ports[i]);
			}
		}
	}

	// Очистка прерывания
	controller->hba_mem->is = controller->hba_mem->is;
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

int ahci_read_lba(struct device *device, uint64_t start, uint64_t count, char *buf)
{
	ahci_port_t* ahci_port = device->dev_data;
	return ahci_port_read_lba(ahci_port, start, count, buf);
}

int ahci_write_lba(struct device *device, uint64_t start, uint64_t count, const char *buf)
{	
	ahci_port_t* ahci_port = device->dev_data;
	return ahci_port_write_lba(ahci_port, start, count, buf);
}

int ahci_device_probe(struct device *dev) 
{
	struct pci_device_info* device_desc = dev->pci_info;

	// Поиск подходящего PCI устройства
	ahci_controller_t* controller = (ahci_controller_t*)kmalloc(sizeof(ahci_controller_t));
	memset(controller, 0, sizeof(ahci_controller_t));

	controller->pci_device = device_desc;

	//Включить прерывания, DMA и MSA
	pci_set_command_reg(device_desc, pci_get_command_reg(device_desc) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
	pci_device_set_enable_interrupts(device_desc, 1);

	if (device_desc->vendor_id == PCI_VENID_JMICRON)
	{
		printk("AHCI: JMicron workaround\n");
	}

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
		printk("AHCI controller reset failed !\n");
	}

	uint8_t irq = alloc_irq(0, "ahci");
    int rc = pci_device_set_msi_vector(dev, irq);
    if (rc != 0) {
        printk("AHCI: Error: Device doesn't support MSI\n");
        return -1;
    }
	register_irq_handler(irq, ahci_irq_handler, controller);

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

	// Буфер под IDENTITY
	char* identity_buffer = pmm_alloc_page();
	char* identity_buffer_virt = (char*) vmm_get_virtual_address((uintptr_t) identity_buffer);

	for (int i = 0; i < 32; i ++) 
	{
		ahci_port_t* cport = &controller->ports[i];
		//Проверка, есть ли устройство
		if (cport->implemented == 0 || cport->present == 0) {
			continue;
		}

		int dt = cport->device_type;
		int link_speed = cport->speed;
		char* link_speed_str = ahci_link_speed_str(link_speed);
		if (dt == AHCI_DEV_SATA)
		{
			printk("SATA drive found at port %i, link speed %s\n", i, link_speed_str);

			//Считать информацию об устройстве
			memset(identity_buffer_virt, 0, IDENTITY_BUFFER_SIZE);
			rc = ahci_port_identity(cport, identity_buffer);

			uint16_t type, capabilities;
			uint32_t cmd_sets;

			// новый вариант
			struct drive_device_info* drive_info = new_drive_device_info();
			
			struct device* drive_dev = new_device();
			drive_dev->dev_type = DEVICE_TYPE_DRIVE;
			device_set_parent(drive_dev, dev);
			device_set_data(drive_dev, cport);
			drive_dev->drive_info = drive_info;

			// Разобрать строку информации
			parse_identity_buffer(identity_buffer_virt, 
									&type,
									&capabilities,
									&cmd_sets,
									&drive_info->sectors,
									drive_dev->dev_name,
									drive_info->serial);

			drive_info->block_size = 512;
			drive_info->nbytes = drive_info->sectors * drive_info->block_size;
			drive_info->uses_lba48 = cmd_sets & (1 << 26);
			drive_info->dev = drive_dev;
			drive_info->write = ahci_write_lba;
			drive_info->read = ahci_read_lba;
			ahci_form_disk_name(drive_info->blockdev_name, 0, i);

			register_device(drive_dev);
		}
		else if (dt == AHCI_DEV_SATAPI)
		{
			printf("SATAPI drive found at port %i, link speed %s\n", i, link_speed_str);
		}
		else if (dt == AHCI_DEV_SEMB)
		{
			printf("SEMB drive found at port %i, link speed %s\n", i, link_speed_str);
		}
		else if (dt == AHCI_DEV_PM)
		{
			printf("PM drive found at port %i, link speed %s\n", i, link_speed_str);
		}
		else
		{
			continue;
		}
	}

	// Освободить IDENTITY
	pmm_free_page(identity_buffer);

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
	.pci_device_ids = ahci_ids,
	.ops = &ahci_ops
};

int ahci_init(void)
{
	register_pci_device_driver(&ahci_driver);
	
	return 0;
}

void ahci_exit(void)
{

}

DEFINE_MODULE("ahci", ahci_init, ahci_exit)