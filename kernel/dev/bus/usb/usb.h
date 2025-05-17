#ifndef _USB_H
#define _USB_H

#include "kairax/types.h"

struct usb_descriptor_header {
    uint8_t bLength;
    uint8_t bDescriptorType;
} PACKED;

struct usb_device_descriptor {
    struct usb_descriptor_header header;
    uint16_t bcdUsb;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
};

struct usb_device_info {
    int d;
};

#endif