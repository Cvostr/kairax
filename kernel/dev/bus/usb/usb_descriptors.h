#ifndef _USB_DESCRIPTORS_H
#define _USB_DESCRIPTORS_H

#include "kairax/types.h"

#define USB_LANGID_ENGLISH_US   0x0409
#define USB_LANGID_ENGLISH_AU   0x0C09
#define USB_LANGID_ENGLISH_CA   0x1009
#define USB_LANGID_GERMAN       0x0407
#define USB_LANGID_FRENCH       0x040C
#define USB_LANGID_SPANISH      0x080A

#define USB_DESCRIPTOR_INTERFACE                        0x04
#define USB_DESCRIPTOR_ENDPOINT                         0x05
#define USB_DESCRIPTOR_INTERFACE_POWER                  0x08
#define USB_DESCRIPTOR_OTG                              0x09
#define USB_DESCRIPTOR_HID                              0x21
#define USB_DESCRIPTOR_SUPERSPEED_ENDPOINT_COMPANION    0x30
#define USB_DESCRIPTOR_SUPERSPEEDPLUS_ISO_ENDPOINT_COMPANION 0x31

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
    uint8_t data[];
} PACKED;

struct usb_interface_descriptor {
    struct usb_descriptor_header header;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} PACKED;

#define USB_ENDPOINT_ADDR_NUMBER_MASK       0b1111
#define USB_ENDPOINT_ADDR_DIRECTION_MASK    (1 << 7)
#define USB_ENDPOINT_ADDR_DIRECTION_IN      (1 << 7)
#define USB_ENDPOINT_ADDR_DIRECTION_OUT     (0 << 7)

// Значения для bmAttributes
#define USB_ENDPOINT_ATTR_TT_MASK       0b11
#define USB_ENDPOINT_ATTR_TT_CONTROL    0b00
#define USB_ENDPOINT_ATTR_TT_ISOCH      0b01
#define USB_ENDPOINT_ATTR_TT_BULK       0b10
#define USB_ENDPOINT_ATTR_TT_INTERRUPT  0b11

#define USB_ENDPOINT_IS_CONTROL(attr) ((attr & USB_ENDPOINT_ATTR_TT_MASK) == USB_ENDPOINT_ATTR_TT_CONTROL)
#define USB_ENDPOINT_IS_BULK(attr) ((attr & USB_ENDPOINT_ATTR_TT_MASK) == USB_ENDPOINT_ATTR_TT_BULK)
#define USB_ENDPOINT_IS_ISOCH(attr) ((attr & USB_ENDPOINT_ATTR_TT_MASK) == USB_ENDPOINT_ATTR_TT_ISOCH)
#define USB_ENDPOINT_IS_INT(attr) ((attr & USB_ENDPOINT_ATTR_TT_MASK) == USB_ENDPOINT_ATTR_TT_INTERRUPT)

#define USB_MAX_HUB_PORTS 31

struct usb_endpoint_descriptor {
    struct usb_descriptor_header header;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} PACKED;

struct usb_ss_ep_companion_descriptor {
	struct usb_descriptor_header header;
	uint8_t bMaxBurst;
	uint8_t bmAttributes;
	uint16_t wBytesPerInterval;
} PACKED;

struct usb_ssp_isoc_ep_companion_descriptor {
    struct usb_descriptor_header header;
	uint16_t wReseved;
	uint32_t dwBytesPerInterval;
} PACKED;

struct usb_hid_descriptor {
    struct usb_descriptor_header header;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    struct {
        uint8_t  bDescriptorType;
        uint16_t wDescriptorLength;
    } PACKED descriptors[];
} PACKED;

struct usb_hub_descriptor {
    uint8_t     bLength;                // Size of this descriptor in bytes
    uint8_t     bDescriptorType;        // USB_DESC_HUB
    uint8_t     bNbrPorts;              // Number of ports
    uint16_t    wHubCharacteristics;    // Hub characteristics
    uint8_t     bPowerOnGood;           // Time (*2ms) from port power on to power good
    uint8_t     bHubContrCurrent;       // Maximum current used by hub controller (mA)
    
    union {
        // Для High Speed
		struct {
			uint8_t  DeviceRemovable[(USB_MAX_HUB_PORTS + 1 + 7) / 8];
			uint8_t  PortPwrCtrlMask[(USB_MAX_HUB_PORTS + 1 + 7) / 8];
		}  PACKED hs;
        // Для Super Speed
		struct {
			uint8_t bHubHdrDecLat;
			uint16_t wHubDelay;
			uint16_t DeviceRemovable;
		}  PACKED ss;
	} hub_info;
} PACKED;

#endif