#include "pci.h"
#include "io.h"
#include "string.h"
#include "memory/paging.h"
#include "stdio.h"

#define MAX_PCI_DEVICES 128

static pci_device_desc pci_devices_descs[MAX_PCI_DEVICES];
int pci_devices_count = 0;

int get_pci_devices_count(){
	return pci_devices_count;
}

pci_device_desc* get_pci_devices_descs(){
	return pci_devices_descs;
}

uint16_t pci_config_read16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset){
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	uint16_t tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
	return tmp;
}

uint32_t pci_config_read32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset){
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	uint32_t tmp = (inl(0xCFC));
	return tmp;
}

void get_pci_bar_desc(uint32 bar, uint8* region_type, uint8* locatable, uint8* prefetchable, uint32* base_addr){
	if(region_type != NULL)
		*region_type = 	bar & 0b1;
	if(locatable != NULL)
		*locatable = 	bar & 0b110;
	if(prefetchable != NULL)
		*prefetchable = bar & 0b1000;
	if(base_addr != NULL)
		*base_addr = 	bar & 0xFFFFFFF0; 
}

int get_pci_device(uint8_t bus, uint8_t device, uint8_t func, pci_device_desc* device_desc){
	uint16_t device_probe = pci_config_read16(bus, device, func, 0);
	//проверка, существует ли устройство
	if(device_probe != 0xFFFF){
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

		uint16 prog_if_revision = pci_config_read16(bus,device, func, 8); //Смещение 8, размер 2 - PIF, Revision
    	device_desc->prog_if = (uint8)((prog_if_revision >> 8) & 0xFF);		//Старшие 8 бит - PIF
		device_desc->revision_id = (uint8)(prog_if_revision & 0xFF);	//Младшие 8 бит - Номер ревизии

		uint16 cache_latency = pci_config_read16(bus,device, func, 12); //Смещение 12, размер 2 - cache line size - latency timer
		device_desc->cache_line_size = (uint8)(cache_latency & 0xFF);
		device_desc->latency_timer = (uint8)((cache_latency >> 8) & 0xFF);

		uint16 bist_header = pci_config_read16(bus,device, func, 14); //Смещение 14, размер 2 - BIST, и тип заголовка
		device_desc->bist = (uint8)((bist_header >> 8) & 0xFF);	  //Старшие 8 бит - BIST
		device_desc->header_type = (uint8)(bist_header & 0xFF);	//Младшие 8 бит - тип заголовка
		//Чтение базовых адресов
		device_desc->BAR0 = pci_config_read32(bus, device, func, 0x10);
		device_desc->BAR1 = pci_config_read32(bus, device, func, 0x14);
		device_desc->BAR2 = pci_config_read32(bus, device, func, 0x18);

		device_desc->BAR3 = pci_config_read32(bus, device, func, 0x1C);
		device_desc->BAR4 = pci_config_read32(bus, device, func, 0x20);
		device_desc->BAR5 = pci_config_read32(bus, device, func, 0x24);

		//uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL | PAGE_DMA ;
		//map_page(get_kernel_pml4(), device_desc->BAR5, pageFlags);

		//printf("%i %i %i %i %i %i\n", device_desc->BAR0, device_desc->BAR1, device_desc->BAR2, device_desc->BAR3, device_desc->BAR4,device_desc->BAR5);

		//Чтение 32х битного указателя на структуру cardbus
		uint16 cardbus_ptr_low = pci_config_read16(bus,device, func, 0x28);
		uint16 cardbus_ptr_high = pci_config_read16(bus,device, func, 0x2A);
		device_desc->cardbus_ptr = (cardbus_ptr_high << 16) | cardbus_ptr_low;

		int16 interrupts = pci_config_read16(bus,device, func, 0x3C); //Смещение 0x3C, размер 2 - данные о прерываниях
		device_desc->interrupt_line = (uint8)(cache_latency & 0xFF);
		device_desc->interrupt_pin = (uint8)((cache_latency >> 8) & 0xFF);

		return 1;
	}
	return 0;
}

void load_pci_devices_list(){
	pci_devices_count = 0;
	memset(pci_devices_descs, 0, sizeof(pci_device_desc) * MAX_PCI_DEVICES);

	for (int bus = 0; bus < 256; bus ++){
		for(uint8_t device = 0; device < 32; device ++){
    			for(uint8_t func = 0; func < 8; func++){
      				int result = get_pci_device(bus, device, func, &pci_devices_descs[pci_devices_count]);
				if(result == 1)
					pci_devices_count++;
			}
     
  		}
	}
}