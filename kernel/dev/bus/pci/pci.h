#ifndef _PCI_H
#define _PCI_H

#include "kairax/types.h"

struct pci_device_bar {
	uintptr_t 		address;
	size_t 			size;
	uint32_t 		flags;
} PACKED;

struct pci_device_info {
 	uint8_t			bus;
  	uint8_t 		device;

  	uint8_t			function;
  	uint16_t 		vendor_id;
  	uint16_t 		device_id;
	uint16_t		status;
  	uint8_t 		device_class;
  	uint8_t 		device_subclass;

  	uint8_t 		prog_if;
	uint8_t 		revision_id;

	uint8_t			cache_line_size;
	uint8_t 		latency_timer;

	uint8_t 		header_type;
	uint8_t 		bist;

	struct pci_device_bar		BAR[6];

	uint32_t 		cardbus_ptr;

	uint8_t 		interrupt_line;
	//uint8_t 		interrupt_pin;
} PACKED;

#define PCI_DEVCMD_BUSMASTER_ENABLE 0x4
#define PCI_DEVCMD_MSA_ENABLE 		0x2
#define PCI_DEVCMD_IO_ENABLE		0x1
#define PCI_DEVCMP_INTERRUPTS_DISABLE (1 << 10)

#define PCI_STATUS_MSI_CAPABLE			(1 << 4)

#define PCI_VENDOR_ID	0
#define PCI_PRODUCT_ID	2

#define PCI_HEADER_TYPE_NORMAL	0
#define PCI_HEADER_TYPE_BRIDGE	0x01
#define PCI_HEADER_TYPE_CARDBUS	0x02
#define PCI_HEADER_TYPE_MULTI	0x80	// Многофункциональное устройство (уже встречали на XHCI)

struct device;

uint16_t i_pci_config_read16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);
uint32_t i_pci_config_read32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);

uint16_t pci_config_read16(struct device* dev, uint32_t offset);
uint32_t pci_config_read32(struct device* dev, uint32_t offset);

void i_pci_config_write16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint16_t data);
void i_pci_config_write32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint32_t data);

void pci_config_write16(struct device* dev, uint32_t offset, uint16_t data);
void pci_config_write32(struct device* dev, uint32_t offset, uint32_t data);

uint16_t pci_get_command_reg(struct pci_device_info* device);

void pci_set_command_reg(struct pci_device_info* device, uint16_t flags);

// Включить или выключить прерывания
void pci_device_set_enable_interrupts(struct pci_device_info* device, int enable);

int pci_device_is_msi_capable(struct pci_device_info* device);

// Назначить устройству номер MSI прерывания
// Включает MSI прерывания
int pci_device_set_msi_vector(struct device* device, uint32_t vector);

// Попыпаться получить устройство по указанным параметрам
// Если устройство есть - будет зарегистрировано через register_device
int probe_pci_device(uint8_t bus, uint8_t device, uint8_t func);

// Сканировать шину PCI и добавить устройства
void load_pci_devices_list();

char* pci_get_device_name(uint8_t class, uint8_t subclass, uint8_t pif);

#endif
