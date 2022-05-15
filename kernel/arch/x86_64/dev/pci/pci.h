#ifndef _PCI_H
#define _PCI_H

#include "stdint.h"

typedef struct {
 	uint8		pci_bus;
  	uint8 		device;

  	uint8		function;
  	uint16 		vendor_id;
  	uint16 		device_id;
	uint16 		command;
	uint16		status;
  	uint8 		device_class;
  	uint8 		device_subclass;

  	uint8 		prog_if;
	uint8 		revision_id;

	uint8		cache_line_size;
	uint8 		latency_timer;

	uint8 		header_type;
	uint8 		bist;

	uint32 		cardbus_ptr;

	uint8 		interrupt_line;
	uint8 		interrupt_pin;
} pci_device_desc;

uint16 pci_config_read16(uint8 bus, uint8 slot, uint8 func, uint8 offset);

int get_pci_device(uint8 bus, uint8 device, uint8 func, pci_device_desc* device_desc);

void load_pci_devices_list();

int get_pci_devices_count();

pci_device_desc* get_pci_devices_descs();

#endif
