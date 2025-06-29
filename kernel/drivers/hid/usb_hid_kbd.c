#include "dev/device_man.h"
#include "hid.h"
#include "kairax/stdio.h"
#include "mem/kheap.h"

struct usb_device_id usb_hid_kbd_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .bInterfaceClass = 3,
        .bInterfaceSubclass = 1, // Supports Boot protocol
        .bInterfaceProtocol = 1 // Keyboard
    },
	{0,}
};

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

int usb_hid_get_report(struct usb_device* device, struct usb_interface* interface, void* report, uint32_t report_length)
{
    uint16_t value = (HID_DESCRIPTOR_REPORT << 8) | 0;

    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = USB_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = value;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = report_length;

    return usb_device_send_request(device, &req, report, report_length);
}

int usb_hid_kbd_device_probe(struct device *dev) 
{   
    struct usb_interface* interface = dev->usb_info.usb_interface;
    struct usb_device* device = dev->usb_info.usb_device;

    struct usb_hid_descriptor* hid_descriptor = interface->hid_descriptor;

    printk("HID keyboard. bcdHID %i, country %i descriptors %i\n", hid_descriptor->bcdHID, hid_descriptor->bCountryCode, hid_descriptor->bNumDescriptors);

    uint16_t hid_descr_length = 0;
    uint16_t report_descr_length = 0;

    for (uint8_t i = 0; i < hid_descriptor->bNumDescriptors; i ++)
    {
        uint8_t descr_type = hid_descriptor->descriptors[i].bDescriptorType;
        uint16_t descr_len = hid_descriptor->descriptors[i].wDescriptorLength;

        switch (descr_type)
        {
        case HID_DESCRIPTOR_REPORT:
            report_descr_length = descr_len;
            break;
        case HID_DESCRIPTOR_HID:
            hid_descr_length = descr_len;
            break;
        default:
            printk("HID: Unknown descriptor type %i\n", descr_type);
            break;
        }
    }

    printk("HID lengths: report %i, hid %i\n", report_descr_length, hid_descr_length);

    // Пока используем Boot
    int rc = usb_hid_set_protocol(device, interface, HID_PROTOCOL_BOOT);
    if (rc != 0)
    {
        printk("HID: SET_PROTOCOL failed (%i)\n", rc);
		return -1;
    }

    uint8_t* report = kmalloc(report_descr_length);
    rc = usb_hid_get_report(device, interface, report, report_descr_length);
    if (rc != 0)
    {
        printk("HID: get report failed (%i)\n", rc);
		return -1;
    }

    kfree(report);

    struct usb_endpoint* interrupt_in_ep = NULL;
    size_t max_packet_sz = 0;

    // Поиск необходимого эндпоинта
    for (uint8_t i = 0; i < interface->descriptor.bNumEndpoints; i ++)
	{
		struct usb_endpoint* ep = &interface->endpoints[i];

		// Нам интересны только Bulk
		if ((ep->descriptor.bmAttributes & USB_ENDPOINT_ATTR_TT_MASK) != USB_ENDPOINT_ATTR_TT_INTERRUPT)
			continue;

		if ((ep->descriptor.bEndpointAddress & USB_ENDPOINT_ADDR_DIRECTION_IN) == USB_ENDPOINT_ADDR_DIRECTION_IN)
		{
			// IN
			interrupt_in_ep = ep;
            max_packet_sz = ep->descriptor.wMaxPacketSize & 0x07FF;
		}
	}

    // Не найдены необходимые endpoints
	if (interrupt_in_ep == NULL)
	{
        printk("HID: No IN Interrupt endpoint\n", rc);
		return -1;
	}

    rc = usb_device_configure_endpoint(device, interrupt_in_ep);
	if (rc != 0)
	{
		printk("HID KBD: Error configuring IN interrupt endpoint (%i)\n", rc);
		return -1;
	}

	return 0;
}

void usb_hid_kbd_device_remove(struct device *dev) 
{
	printk("HID KBD: Device Remove\n");
	return 0;
}

struct device_driver_ops usb_hid_kbd_ops = {

    .probe = usb_hid_kbd_device_probe,
    .remove = usb_hid_kbd_device_remove
    //void (*shutdown) (struct device *dev);
};

struct usb_device_driver usb_hid_kbd_storage_driver = {
	.device_ids = usb_hid_kbd_ids,
	.ops = &usb_hid_kbd_ops
};

void usb_hid_kbd_init()
{
	register_usb_device_driver(&usb_hid_kbd_storage_driver);

}
