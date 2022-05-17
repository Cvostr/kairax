#ifndef _AHCI_H
#define _AHCI_H

#include "dev/pci/pci.h"

uint64_t abar_pci_dev(pci_device_desc* pci_device);

void ahci_init();

#endif
