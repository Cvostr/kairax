#ifndef _DEVICE_H
#define _DEVICE_H

#include "bus/pci/pci.h"
#include "bus/usb/usb.h"
#include "type/drive_device.h"
#include "type/net_device.h"
#include "kairax/guid/guid.h"

#define DEVICE_NAME_LEN         60

#define DEVICE_TYPE_UNKNOWN         0
#define DEVICE_TYPE_ANY             0xFFFF
#define DEVICE_TYPE_DISK_CONTROLLER 1
#define DEVICE_TYPE_DRIVE           2
#define DEVICE_TYPE_DRIVE_PARTITION 3
#define DEVICE_TYPE_AUDIO_ENDPOINT  4
#define DEVICE_TYPE_USB_CONTROLLER  5
#define DEVICE_TYPE_NETWORK_ADAPTER 6
#define DEVICE_TYPE_USB_COMPOSITE   7

#define DEVICE_BUS_NONE     0       
#define DEVICE_BUS_PCI      1
#define DEVICE_BUS_USB      2

#define DEVICE_STATE_UNINITIALIZED 1
#define DEVICE_STATE_INITIALIZED   2
#define DEVICE_STATE_INIT_ERROR    3

struct device {

    guid_t          id;
    int             dev_type;
    char            dev_name[DEVICE_NAME_LEN + 1];
    int             dev_bus;    
    struct device*  dev_parent;
    int             dev_state;
    int             dev_status_code;
    void*           dev_data;
    void*           dev_driver;

    // В зависимости от шины
    union {
        struct pci_device_info* pci_info;
        struct {
            struct usb_device* usb_device;
            struct usb_interface* usb_interface;
        } usb_info;
    };

    // В зависимости от типа
    union {
        struct drive_device_info* drive_info;
        struct nic* nic;
    };
};

struct device* new_device();
void device_set_name(struct device* dev, const char* name);

int drive_device_read( struct device* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            char* buffer);

int drive_device_write( struct device* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            char* buffer);

#endif