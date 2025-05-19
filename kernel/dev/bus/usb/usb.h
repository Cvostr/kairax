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
} PACKED;

struct usb_string_language_descriptor {
    struct usb_descriptor_header header;
    uint16_t lang_ids[126];
} PACKED;

struct usb_string_descriptor {
    struct usb_descriptor_header header;
    uint16_t unicode_string[126];
} PACKED;

struct usb_configuration_descriptor {
    struct usb_descriptor_header header;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
    uint8_t data[245];
} PACKED;

struct usb_device_info {
    int d;
};

#endif