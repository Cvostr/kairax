#ifndef _USB_H
#define _USB_H

#include "kairax/types.h"
#include "usb_descriptors.h"

struct usb_device_info {
    int d;
};

struct usb_endpoint {
    struct usb_endpoint_descriptor descriptor;
    struct usb_ss_ep_companion_descriptor super_speed_companion;
};

struct usb_interface {
    struct usb_interface_descriptor descriptor;
    struct usb_endpoint* endpoints;
};

struct usb_interface* new_usb_interface(struct usb_interface_descriptor* descriptor);
struct usb_endpoint* new_usb_endpoint_array(size_t endpoints);

#endif