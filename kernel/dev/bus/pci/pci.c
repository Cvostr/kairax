#include "pci.h"
#include "io.h"
#include "string.h"
#include "memory/paging.h"
#include "stdio.h"
#include "mem/kheap.h"
#include "dev/device_man.h"

#define PCI_BAR_IO 0x01
#define PCI_BAR_LOWMEM 0x02
#define PCI_BAR_64 0x04
#define PCI_BAR_PREFETCH 0x08

uint16_t i_pci_config_read16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset)
{
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	uint16_t tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
	return tmp;
}

uint32_t i_pci_config_read32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset)
{
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

	outl(0xCF8, address);
	uint32_t tmp = (inl(0xCFC));
	return tmp;
}

uint16_t pci_config_read16(struct device* dev, uint32_t offset)
{
	struct pci_device_info* pci = dev->pci_info;
	return i_pci_config_read16(pci->bus, pci->device, pci->function, offset);
}

uint32_t pci_config_read32(struct device* dev, uint32_t offset)
{
	struct pci_device_info* pci = dev->pci_info;
	return i_pci_config_read32(pci->bus, pci->device, pci->function, offset);
}

void i_pci_config_write32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint32_t data)
{
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
	outl(0xCF8, address);
	outl(0xCFC, data);
}

void i_pci_config_write16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint16_t data)
{
	uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
	outl(0xCF8, address);
	outw(0xCFC, data);
}

void pci_config_write16(struct device* dev, uint32_t offset, uint16_t data)
{
	struct pci_device_info* pci = dev->pci_info;
	return i_pci_config_write16(pci->bus, pci->device, pci->function, offset, data);
}

void pci_config_write32(struct device* dev, uint32_t offset, uint32_t data)
{
	struct pci_device_info* pci = dev->pci_info;
	return i_pci_config_write32(pci->bus, pci->device, pci->function, offset, data);
}

void read_pci_bar(uint32_t bus, uint32_t device, uint32_t func, uint32_t bar_index, uint32_t* address, uint32_t* mask)
{
	uint32_t offset = 0x10 + 4 * bar_index;
	*address = i_pci_config_read32(bus, device, func, offset);
	i_pci_config_write32(bus, device, func, offset, 0xffffffff);

	*mask = i_pci_config_read32(bus, device, func, offset);
	i_pci_config_write32(bus, device, func, offset, *address);
}

void pci_set_command_reg(struct pci_device_info* device, uint16_t flags)
{
	i_pci_config_write16(device->bus, device->device, device->function, 0x4, flags);
}

uint16_t pci_get_command_reg(struct pci_device_info* device)
{
	return i_pci_config_read16(device->bus, device->device, device->function, 0x4);
}

void pci_device_set_enable_interrupts(struct pci_device_info* device, int enable)
{
	uint16_t cmd = pci_get_command_reg(device);
	if (enable > 0) {
		cmd = cmd & ~(PCI_DEVCMP_INTERRUPTS_DISABLE);
	} else {
		cmd = cmd | PCI_DEVCMP_INTERRUPTS_DISABLE;
	}

	pci_set_command_reg(device, cmd);
}

int probe_pci_device(uint8_t bus, uint8_t device, uint8_t func)
{
	uint16_t device_probe = i_pci_config_read16(bus, device, func, 0);
	//проверка, существует ли устройство
	if (device_probe != 0xFFFF) {

		struct pci_device_info* device_desc = kmalloc(sizeof(struct pci_device_info));
		memset(device_desc, 0, sizeof(struct pci_device_info));

		device_desc->bus = bus; //шина устройства
		device_desc->device = device; //номер устройства
    	device_desc->function = func;

    	device_desc->vendor_id = i_pci_config_read16(bus,device, func, 0); //Смещение 0, размер 2б - номер производителя
    	device_desc->device_id = i_pci_config_read16(bus,device, func, 2); //Смещение 2, размер 2 - ID устройства
		device_desc->status = i_pci_config_read16(bus,device, func, 6); //Смещение 6, размер 2 - Статус
    	uint16_t devclass = i_pci_config_read16(bus,device, func, 10);  //Смещение 10, размер 2 (Класс - Подкласс)
    	device_desc->device_class = (uint8_t)((devclass >> 8) & 0xFF);		//Старшие 8 бит - класс
    	device_desc->device_subclass = (uint8_t)(devclass & 0xFF);		//Младшие 8 бит - подкласс

		uint16_t prog_if_revision = i_pci_config_read16(bus,device, func, 8); //Смещение 8, размер 2 - PIF, Revision
    	device_desc->prog_if = (uint8_t)((prog_if_revision >> 8) & 0xFF);		//Старшие 8 бит - PIF
		device_desc->revision_id = (uint8_t)(prog_if_revision & 0xFF);	//Младшие 8 бит - Номер ревизии

		uint16_t cache_latency = i_pci_config_read16(bus,device, func, 12); //Смещение 12, размер 2 - cache line size - latency timer
		device_desc->cache_line_size = (uint8_t)(cache_latency & 0xFF);
		device_desc->latency_timer = (uint8_t)((cache_latency >> 8) & 0xFF);

		uint16_t bist_header = i_pci_config_read16(bus,device, func, 14); //Смещение 14, размер 2 - BIST, и тип заголовка
		device_desc->bist = (uint8_t)((bist_header >> 8) & 0xFF);	  //Старшие 8 бит - BIST
		device_desc->header_type = (uint8_t)(bist_header & 0xFF);	//Младшие 8 бит - тип заголовка
		//Чтение базовых адресов

		if (device_desc->header_type == 0) {

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

					bar_ptr->address = (uintptr_t)(((uint64_t)addressHigh << 32) | (address & ~0xf));
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
			device_desc->cardbus_ptr = i_pci_config_read32(bus, device, func, 0x28);

			//uint16_t interrupts = i_pci_config_read16(bus,device, func, 0x3C); //Смещение 0x3C, размер 2 - данные о прерываниях
			//device_desc->interrupt_line = (uint8_t)(interrupts & 0xFF);
			//device_desc->interrupt_pin = (uint8_t)((interrupts >> 8) & 0xFF);
			//Отключение прерываний у устройства
			pci_device_set_enable_interrupts(device_desc, 0);
		}

		char* dev_basename = pci_get_device_name(device_desc->device_class, device_desc->device_subclass, device_desc->prog_if);

		struct device* dev = kmalloc(sizeof(struct device));
		memset(dev, 0, sizeof(struct device));

		dev->dev_bus = DEVICE_BUS_PCI;
		dev->pci_info = device_desc;

		if (dev_basename != NULL) {
			strcpy(dev->dev_name, dev_basename);
		}

		register_device(dev);

		return 0;
	}
	
	return -1;
}

extern uint64_t arch_get_msi_address(uint64_t *data, size_t vector, uint32_t processor, uint8_t edgetrigger, uint8_t deassert);

int pci_device_is_msi_capable(struct pci_device_info* device)
{
	return (device->status & PCI_STATUS_MSI_CAPABLE) == PCI_STATUS_MSI_CAPABLE;
}

int pci_device_set_msi_vector(struct device* device, uint32_t vector)
{
	if (pci_device_is_msi_capable(device->pci_info) == 0) {
		return -1;
	}

	uint32_t cap_reg = pci_config_read32(device, 0x34);
	cap_reg &= 0xFF;

	while (cap_reg != 0) {
		uint32_t cap = pci_config_read32(device, cap_reg);

		uint8_t type = cap & 0xFF;

		if (type == 0x05) {
			uint16_t messctl = cap >> 16;
			int msi_64bit_cap = (messctl & (1 << 7));
			int mask = (messctl & (1 << 8));

			uint64_t msi_data = 0;
			uint64_t msi_addr = arch_get_msi_address(&msi_data, vector, 0, 1, 0);

			// Записать адрес и msi_data
			pci_config_write32(device, cap_reg + 0x04, msi_addr & UINT32_MAX);

			if (msi_64bit_cap) {
				pci_config_write32(device, cap_reg + 0x8, msi_addr >> 32);
				pci_config_write16(device, cap_reg + 0xC, msi_data & UINT16_MAX);
			} else {
				pci_config_write16(device, cap_reg + 0x8, msi_data & UINT16_MAX);
			}

			if (mask)
				pci_config_write32(device, cap_reg + 0x10, 0);

			// Записать Enable Bit
			messctl |= 1;
			cap = (((uint32_t) messctl) << 16) | cap & UINT16_MAX;
			pci_config_write32(device, cap_reg, cap);

			return 0;
		}

		cap_reg = (cap >> 8) & 0xFF;
	}
}

char* pci_get_device_name(uint8_t class, uint8_t subclass, uint8_t pif)
{
	char* result = NULL;
	switch(class){
        case 0x1: {//Mass storage
            switch(subclass){ //IDE contrl
                case 0x1: 
					result = "Mass Storage IDE Controller"; 
                    break;
                case 0x6:
                    if (pif == 0x1) {
                        result = "Mass Storage SATA AHCI controller";
                    } else {
                        result = "Mass Storage SATA controller";
                    } 
                    break;
                case 0x8: 
					result = "Mass Storage NVME controller"; 
                    break;
                }
                
            break;
            }
            case 0x2: {//Internet
            switch(subclass) {
                case 0x0:  //Ethernet
					result = "Network Ethernet controller"; 
					break;  
                }
            break;
            }

            case 0x3:{ //Graphics
            switch(subclass){ 
                case 0x0: result = "VGA graphics controller"; break;//VGA
                case 0x02: result = "3D graphics controller"; break;
                case 0x80: result = "Unknown graphics card"; break;
                }
            break;
            }

            case 0x4:{ //Multimedia
            switch(subclass){ 
                case 0x03: result = "Multimedia Audio device"; break;
                }
            break;
            }

            case 0x5:{ //Multimedia
            switch(subclass){ 
                case 0x80: result = "Unknown Memory controller"; break;
                }
            break;
            }
            case 0x6:{ //System bridge devices
            switch(subclass){ //
                case 0x0: result = "System host bridge"; break;
                case 0x1: result = "System ISA bridge"; break;
                case 0x4: result = "System PCI-PCI bridge"; break;
                case 0x80: result = "System unknown bridge"; break;
                }
            break;
            }

            case 0x7:{ //Simple communication port devices
            switch(subclass){ //
                case 0x0: result = "System host bridge"; break;
                case 0x1: result = "System ISA bridge"; break;
                case 0x4: result = "System PCI-PCI bridge"; break;
                case 0x80: result = "Unknown Simple Communications dev"; break;
                }
            break;
            }

            case 0xC:{ //Serial port devices
            switch(subclass){ //
                case 0x3: result = "USB Controller"; break;
                case 0x5: result = "SMBus"; break;
                }
            break;
            }
        }

	return result;
}

void load_pci_devices_list()
{
	for (int bus = 0; bus < 256; bus ++) {
		for (uint8_t device = 0; device < 32; device ++) {
    		for (uint8_t func = 0; func < 8; func++) {
      			probe_pci_device(bus, device, func);
			}
  		}
	}
}