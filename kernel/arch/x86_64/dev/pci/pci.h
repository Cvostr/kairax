#ifndef _PCI_H
#define _PCI_H

#include "stdint.h"

typedef struct PACKED {
	uintptr_t 	address;
	size_t 		size;
	uint32_t 		flags;
}pci_bar_t;

typedef struct PACKED {
 	uint8_t			bus;
  	uint8_t 		device;

  	uint8_t			function;
  	uint16_t 		vendor_id;
  	uint16_t 		device_id;
	uint16_t 		command;
	uint16_t		status;
  	uint8_t 		device_class;
  	uint8_t 		device_subclass;

  	uint8_t 		prog_if;
	uint8_t 		revision_id;

	uint8_t			cache_line_size;
	uint8_t 		latency_timer;

	uint8_t 		header_type;
	uint8_t 		bist;

	pci_bar_t		BAR[6];

	uint32_t 		cardbus_ptr;

	uint8_t 		interrupt_line;
	uint8_t 		interrupt_pin;
} pci_device_desc;

uint16_t pci_config_read16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);

uint32_t pci_config_read32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);

void pci_config_write32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint32_t data);

void pci_enable_busmaster(pci_device_desc* device);

int get_pci_device(uint8_t bus, uint8_t device, uint8_t func, pci_device_desc* device_desc);

void load_pci_devices_list();

int get_pci_devices_count();

pci_device_desc* get_pci_devices_descs();

#endif
