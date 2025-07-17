#ifndef _HID_H
#define _HID_H

#include "dev/bus/usb/usb.h"

#define HID_SET_REPORT   0x09
#define HID_SET_IDLE     0x0A
#define HID_SET_PROTOCOL 0x0B

#define HID_PROTOCOL_BOOT   0
#define HID_PROTOCOL_REPORT 1

#define HID_DESCRIPTOR_HID 0x21
#define HID_DESCRIPTOR_REPORT 0x22
#define HID_DESCRIPTOR_PHYSICAL 0x23

int usb_hid_set_idle(struct usb_device* device, struct usb_interface* interface);
int usb_hid_get_report(struct usb_device* device, struct usb_interface* interface, uint8_t report_type, uint8_t report_id, void* report, uint32_t report_length);
int usb_hid_set_protocol(struct usb_device* device, struct usb_interface* interface, int protocol);

#endif