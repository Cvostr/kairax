#include "pci.h"
#include "io.h"
#include "string.h"
#include "memory/paging.h"
#include "stdio.h"
#include "mem/kheap.h"

#define MAX_PCI_DEVICES 128

#define PCI_BAR_IO 0x01
#define PCI_BAR_LOWMEM 0x02
#define PCI_BAR_64 0x04
#define PCI_BAR_PREFETCH 0x08

static struct pci_device_desc* pci_devices_descs;
int pci_devices_count = 0;

int get_pci_devices_count()
{
	return pci_devices_count;
}

struct pci_device_desc* get_pci_devices_descs()
{
	return pci_devices_descs;
}

uint16_t pci_config_read16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset)
{
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	uint16_t tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
	return tmp;
}

uint32_t pci_config_read32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset)
{
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	uint32_t tmp = (inl(0xCFC));
	return tmp;
}

void pci_config_write32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint32_t data)
{
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
	outl(0xCF8, address);
	outl(0xCFC, data);
}

void pci_config_write16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint16_t data)
{
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
	outl(0xCF8, address);
	outw(0xCFC, data);
}

void read_pci_bar(uint32_t bus, uint32_t device, uint32_t func, uint32_t bar_index, uint32_t* address, uint32_t* mask)
{
	uint32_t offset = 0x10 + 4 * bar_index;
	*address = pci_config_read32(bus, device, func, offset);
	pci_config_write32(bus, device, func, offset, 0xffffffff);

	*mask = pci_config_read32(bus, device, func, offset);
	pci_config_write32(bus, device, func, offset, *address);
}

void pci_set_command_reg(struct pci_device_desc* device, uint16_t flags)
{
	pci_config_write16(device->bus, device->device, device->function, 0x4, flags);
}

uint16_t pci_get_command_reg(struct pci_device_desc* device)
{
	return pci_config_read16(device->bus, device->device, device->function, 0x4);
}

void pci_device_set_enable_interrupts(struct pci_device_desc* device, int enable)
{
	uint16_t cmd = pci_get_command_reg(device);
	if (enable > 0) {
		cmd = cmd & ~(PCI_DEVCMP_INTERRUPTS_DISABLE);
	} else {
		cmd = cmd | PCI_DEVCMP_INTERRUPTS_DISABLE;
	}

	pci_set_command_reg(device, cmd);
}

int get_pci_device(uint8_t bus, uint8_t device, uint8_t func, struct pci_device_desc* device_desc)
{
	uint16_t device_probe = pci_config_read16(bus, device, func, 0);
	//проверка, существует ли устройство
	if (device_probe != 0xFFFF){
		device_desc->bus = bus; //шина устройства
		device_desc->device = device; //номер устройства
    	device_desc->function = func;

    	device_desc->vendor_id = pci_config_read16(bus,device, func, 0); //Смещение 0, размер 2б - номер производителя
    	device_desc->device_id = pci_config_read16(bus,device, func, 2); //Смещение 2, размер 2 - ID устройства
		device_desc->command = pci_config_read16(bus,device, func, 4); //Смещение 4, размер 2 - Номер команды
		device_desc->status = pci_config_read16(bus,device, func, 6); //Смещение 6, размер 2 - Статус
    	uint16_t devclass = pci_config_read16(bus,device, func, 10);  //Смещение 10, размер 2 (Класс - Подкласс)
    	device_desc->device_class = (uint8_t)((devclass >> 8) & 0xFF);		//Старшие 8 бит - класс
    	device_desc->device_subclass = (uint8_t)(devclass & 0xFF);		//Младшие 8 бит - подкласс

		uint16_t prog_if_revision = pci_config_read16(bus,device, func, 8); //Смещение 8, размер 2 - PIF, Revision
    	device_desc->prog_if = (uint8_t)((prog_if_revision >> 8) & 0xFF);		//Старшие 8 бит - PIF
		device_desc->revision_id = (uint8_t)(prog_if_revision & 0xFF);	//Младшие 8 бит - Номер ревизии

		uint16_t cache_latency = pci_config_read16(bus,device, func, 12); //Смещение 12, размер 2 - cache line size - latency timer
		device_desc->cache_line_size = (uint8_t)(cache_latency & 0xFF);
		device_desc->latency_timer = (uint8_t)((cache_latency >> 8) & 0xFF);

		uint16_t bist_header = pci_config_read16(bus,device, func, 14); //Смещение 14, размер 2 - BIST, и тип заголовка
		device_desc->bist = (uint8_t)((bist_header >> 8) & 0xFF);	  //Старшие 8 бит - BIST
		device_desc->header_type = (uint8_t)(bist_header & 0xFF);	//Младшие 8 бит - тип заголовка
		//Чтение базовых адресов

		if (device_desc->header_type != 0)
			return 0;

		for (int i = 0; i < 6; i ++){
			uint32_t address = 0;
			uint32_t mask = 0;
			read_pci_bar(bus, device, func, i, &address, &mask);

			struct pci_device_bar* bar_ptr = &device_desc->BAR[i];

			if (mask & PCI_BAR_64) {
				// 64-bit MMIO
				uint32_t addressHigh;
				uint32_t maskHigh;
				read_pci_bar(bus, device, func, i + 1, &addressHigh, &maskHigh);

				bar_ptr->address = (uintptr_t)(((uint32_t)addressHigh << 16) | (address & ~0xf));
				bar_ptr->size = ~(((uint32_t)maskHigh << 16) | (mask & ~0xf)) + 1;
				bar_ptr->flags = address & 0xf;
			} else if (mask & PCI_BAR_IO) {
				bar_ptr->address = ((uint32_t)address & 0xFFFFFFFC);
			} else {
				// 32-bit MMIO
				bar_ptr->address = (uintptr_t)(uint32_t)(address & ~0xf);
				bar_ptr->size = ~(mask & ~0xf) + 1;
				bar_ptr->flags = mask & 0xf;
			}

		}

		//Чтение 32х битного указателя на структуру cardbus
		device_desc->cardbus_ptr = pci_config_read32(bus, device, func, 0x28);

		uint16_t interrupts = pci_config_read16(bus,device, func, 0x3C); //Смещение 0x3C, размер 2 - данные о прерываниях
		device_desc->interrupt_line = (uint8_t)(interrupts & 0xFF);
		device_desc->interrupt_pin = (uint8_t)((interrupts >> 8) & 0xFF);
		//Отключение прерываний у устройства
		pci_device_set_enable_interrupts(device_desc, 0);

		return 1;
	}
	
	return 0;
}

void load_pci_devices_list()
{
	pci_devices_count = 0;
	pci_devices_descs = kmalloc(sizeof(struct pci_device_desc) * MAX_PCI_DEVICES);
	memset(pci_devices_descs, 0, sizeof(struct pci_device_desc) * MAX_PCI_DEVICES);

	for (int bus = 0; bus < 256; bus ++) {
		for (uint8_t device = 0; device < 32; device ++) {
    		for (uint8_t func = 0; func < 8; func++) {
      			int result = get_pci_device(bus, device, func, &pci_devices_descs[pci_devices_count]);
				if (result == 1)
					pci_devices_count++;
			}
     
  		}
	}
}