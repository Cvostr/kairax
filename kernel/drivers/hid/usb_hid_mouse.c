#include "dev/device_man.h"
#include "hid.h"
#include "kairax/stdio.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "mem/iomem.h"

struct usb_device_id usb_hid_mouse_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .bInterfaceClass = 3,
        .bInterfaceSubclass = 1, // Supports Boot protocol
        .bInterfaceProtocol = 2 // Mouse
    },
	{0,}
};

struct usb_hid_mouse {
    struct usb_device* device;
    struct usb_endpoint* ep;

    size_t                              report_buffer_pages;
    uintptr_t                           report_buffer_phys;
    void*                               report_buffer;
};

void mouse_event_callback(struct usb_msg* msg)
{
    struct usb_hid_mouse* kbd = msg->private;
    struct hid_kbd_boot_int_report* rep = kbd->report_buffer;

    printk("Mouse moved\n");

    usb_send_async_msg(kbd->device, kbd->ep, msg);
}

int usb_hid_mouse_device_probe(struct device *dev) 
{   
    struct usb_interface* interface = dev->usb_info.usb_interface;
    struct usb_device* device = dev->usb_info.usb_device;

    // Эндпоинт
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
            printk("HID Mouse: interrupt ep max pack size %i\n", ep->descriptor.wMaxPacketSize);
            break;
		}
	}

    // Не найдены необходимые endpoints
	if (interrupt_in_ep == NULL)
	{
        printk("HID Mouse: No IN Interrupt endpoint\n");
		return -1;
	}

    // Пока используем Boot
    int rc = usb_hid_set_protocol(device, interface, HID_PROTOCOL_BOOT);
    if (rc != 0)
    {
        printk("HID Mouse: SET_PROTOCOL failed (%i)\n", rc);
		return -1;
    }

    rc = usb_device_configure_endpoint(device, interrupt_in_ep);
	if (rc != 0)
	{
		printk("HID Mouse: Error configuring IN interrupt endpoint (%i)\n", rc);
		return -1;
	}

    // SET_IDLE
    usb_hid_set_idle(device, interface);

    // GET_REPORT
    uint8_t report[8];
    rc = usb_hid_get_report(device, interface, 0x01, 0, report, 8);
    if (rc != 0)
    {
        printk("HID Mouse: get report failed (%i)\n", rc);
		return -1;
    }

    struct usb_hid_mouse* kbd = kmalloc(sizeof(struct usb_hid_mouse));
    kbd->device = device;
    kbd->ep = interrupt_in_ep;

    // Выделить память под буфер report
    kbd->report_buffer_phys = (uintptr_t) pmm_alloc(max_packet_sz, &kbd->report_buffer_pages);
    kbd->report_buffer = map_io_region(kbd->report_buffer_phys, kbd->report_buffer_pages * PAGE_SIZE);

    struct usb_msg* msg = kmalloc(sizeof(struct usb_msg));
    msg->data = kbd->report_buffer_phys;
    msg->length = max_packet_sz;
    msg->callback = mouse_event_callback;
    msg->private = kbd;

    usb_send_async_msg(device, interrupt_in_ep, msg);

	return 0;
}

void usb_hid_mouse_device_remove(struct device *dev) 
{
	printk("HID Mouse: Device Remove\n");
	return 0;
}

struct device_driver_ops usb_hid_mouse_ops = {

    .probe = usb_hid_mouse_device_probe,
    .remove = usb_hid_mouse_device_remove
    //void (*shutdown) (struct device *dev);
};

struct usb_device_driver usb_hid_mouse_driver = {
	.device_ids = usb_hid_mouse_ids,
	.ops = &usb_hid_mouse_ops
};

void usb_hid_mouse_init()
{
	register_usb_device_driver(&usb_hid_mouse_driver);
}
