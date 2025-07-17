#include "hid.h"

int usb_hid_set_protocol(struct usb_device* device, struct usb_interface* interface, int protocol)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = HID_SET_PROTOCOL;
	req.wValue = protocol;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = 0;

    return usb_device_send_request(device, &req, NULL, 0);
}

int usb_hid_set_idle(struct usb_device* device, struct usb_interface* interface)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = HID_SET_IDLE;
	req.wValue = 0;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = 0;

    return usb_device_send_request(device, &req, NULL, 0);
}

int usb_hid_get_report(struct usb_device* device, struct usb_interface* interface, uint8_t report_type, uint8_t report_id, void* report, uint32_t report_length)
{
    uint16_t value = (((uint16_t) report_type) << 8) | report_id;

    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = 1;
	req.wValue = value;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = report_length;

    return usb_device_send_request(device, &req, report, report_length);
}