#ifndef _DEVICE_H
#define _DEVICE_H

#include "bus/pci/pci.h"
#include "bus/usb/usb.h"

#include "type/drive_device.h"

#define DEVICE_NAME_LEN     60

#define DEVICE_TYPE_UNKNOWN         0
#define DEVICE_TYPE_DISK_CONTROLLER 1
#define DEVICE_TYPE_DRIVE           2
#define DEVICE_TYPE_DRIVE_PARTITION 3
#define DEVICE_TYPE_AUDIO           4
#define DEVICE_TYPE_USB_CONTROLLER  5

#define DEVICE_BUS_NONE     0       
#define DEVICE_BUS_PCI      1
#define DEVICE_BUS_USB      2

#define DEVICE_STATE_UNINITIALIZED 1
#define DEVICE_STATE_INITIALIZED   2
#define DEVICE_STATE_INIT_ERROR    3

struct device {

    int             dev_type;
    char            dev_name[DEVICE_NAME_LEN];
    int             dev_bus;    
    struct device*  dev_parent;
    int             dev_state;
    int             dev_status_code;
    void*           dev_data;
    void*           dev_driver;

    union {
        struct pci_device_info* pci_info;
        struct usb_device_info* usb_info;
    };

    union {
        struct drive_device* drive_info;
    };
};

#endif