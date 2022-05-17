#ifndef _PCI_H
#define _PCI_H

#include "stdint.h"

typedef struct PACKED {
	uintptr_t 	address;
	size_t 		size;
	int 		flags;
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

	uint32_t		BAR0;
	uint32_t		BAR1;
	uint32_t		BAR2;
	uint32_t		BAR3;
	uint32_t		BAR4;
	uint32_t		BAR5;

	uint32_t 		cardbus_ptr;

	uint8_t 		interrupt_line;
	uint8_t 		interrupt_pin;
} pci_device_desc;

uint16_t pci_config_read16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);

int get_pci_device(uint8_t bus, uint8_t device, uint8_t func, pci_device_desc* device_desc);

void load_pci_devices_list();

int get_pci_devices_count();

pci_device_desc* get_pci_devices_descs();

void get_pci_bar_desc(uint32_t bar, uint8* region_type, uint8* locatable, uint8* prefetchable, uint32* base_addr);

#endif
