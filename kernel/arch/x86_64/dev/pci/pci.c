#include "pci.h"
#include "io.h"
#include "string.h"

#define MAX_PCI_DEVICES 128

static pci_device_desc pci_devices_descs[MAX_PCI_DEVICES];
int pci_devices_count = 0;

int get_pci_devices_count(){
	return pci_devices_count;
}

pci_device_desc* get_pci_devices_descs(){
	return pci_devices_descs;
}

uint16 pci_config_read16(uint8 bus, uint8 slot, uint8 func, uint8 offset){
	uint32 lbus  = (uint32)bus;
    	uint32 lslot = (uint32)slot;
    	uint32 lfunc = (uint32)func;
	uint32 address = (uint32)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32)0x80000000));

	outl(0xCF8, address);
	return (uint16)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
}

int get_pci_device(uint8 bus, uint8 device, uint8 func, pci_device_desc* device_desc){
	uint16 device_probe = pci_config_read16(bus, device, func, 0);
	//проверка, существует ли устройство
	if(device_probe != 0xFFFF){
		device_desc->pci_bus = bus; //шина устройства
		device_desc->device = device; //номер устройства
    		device_desc->function = func;

    		device_desc->vendor_id = pci_config_read16(bus,device, func, 0); //Смещение 0, размер 2б - номер производителя
    		device_desc->device_id = pci_config_read16(bus,device, func, 2); //Смещение 2, размер 2 - ID устройства
		device_desc->command = pci_config_read16(bus,device, func, 4); //Смещение 4, размер 2 - Номер команды
		device_desc->status = pci_config_read16(bus,device, func, 6); //Смещение 6, размер 2 - Статус
    		uint16 devclass = pci_config_read16(bus,device, func, 10);  //Смещение 10, размер 2 (Класс - Подкласс)
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
		for(uint8 device = 0; device < 32; device ++){
    			for(uint8 func = 0; func < 8; func++){
      				int result = get_pci_device(bus, device, func, &pci_devices_descs[pci_devices_count]);
				if(result == 1)
					pci_devices_count++;
			}
     
  		}
	}
}