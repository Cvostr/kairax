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

struct usb_device_id {
    uint16_t    match_flags;
    // Для поиска по продукту
    uint16_t    idVendor;
    uint16_t    idProduct;
    uint16_t    bcdDevice;
    // Для поиска по классу
    uint8_t     bDeviceClass;
    uint8_t     bDeviceSubclass;
    uint8_t     bDeviceProtocol;
    // Для поиска по классу интерфейса
    uint8_t     bInterfaceClass;
    uint8_t     bInterfaceSubclass;
    uint8_t     bIntefaceProtocol;
};

// USB - Для поиска по продукту
#define USB_DEVICE_ID_MATCH_VENDOR          0b000000001
#define USB_DEVICE_ID_MATCH_PRODUCT         0b000000010
#define USB_DEVICE_ID_MATCH_BCDDEVICE       0b000000100
// USB - Для поиска по классу
#define USB_DEVICE_ID_MATCH_DEV_CLASS       0b000001000
#define USB_DEVICE_ID_MATCH_DEV_SUBCLASS    0b000010000
#define USB_DEVICE_ID_MATCH_DEV_PROTOCOL    0b000100000
// USB - для поиска по интерфейсу
#define USB_DEVICE_ID_MATCH_INT_CLASS       0b001000000
#define USB_DEVICE_ID_MATCH_INT_SUBCLASS    0b010000000
#define USB_DEVICE_ID_MATCH_INT_PROTOCOL    0b100000000

#define USB_DEVICE(vendor, product) \
	.match_flags = (USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_PRODUCT), \
	.idVendor = (vendor), \
	.idProduct = (product)

struct usb_device_driver {
    char* driver_name;
    struct usb_device_id* device_ids;
    struct device_driver_ops* ops;
};

int register_usb_device_driver(struct usb_device_driver* driver);
struct usb_device_driver* drivers_get_for_usb_device(struct usb_device_id* usb_dev_id);

#endif