#ifndef _DEVICE_DRIVER_H
#define _DEVICE_DRIVER_H

#include "kairax/types.h"
#include "device.h"

#define PCI_ANY_ID (uint16_t) (~0)

struct device_driver_ops {

    int (*probe) (struct device *dev);
    void (*remove) (struct device *dev);
    void (*shutdown) (struct device *dev);
};

struct pci_device_id {
	uint16_t	vendor_id;
	uint16_t 	dev_id;
    uint8_t     dev_class;
    uint8_t     dev_subclass;
    uint8_t     dev_pif;
};

#define PCI_DEVICE(vendor, device) .vendor_id = vendor, .dev_id = device
#define PCI_DEVICE_CLASS(dclass, subclass, pif) .vendor_id = PCI_ANY_ID, .dev_id = PCI_ANY_ID, .dev_class = dclass, .dev_subclass = subclass, .dev_pif = pif

struct pci_device_driver {

    char* dev_name;
    struct pci_device_id* pci_device_ids;
    struct device_driver_ops* ops;
};

int register_pci_device_driver(struct pci_device_driver* driver);

struct pci_device_driver* drivers_get_for_pci_device(struct pci_device_id* pci_dev_id);

#endif