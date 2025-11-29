#include "dev/device_man.h"
#include "hid.h"
#include "kairax/stdio.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "mem/iomem.h"
#include "string.h"
#include "drivers/char/input/mouse.h"

struct usb_device_id usb_hid_mouse_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .bInterfaceClass = 3,
        .bInterfaceProtocol = 2 // Mouse
    },
	{0,}
};

struct usb_hid_mouse {
    struct usb_device* device;
    struct usb_endpoint* ep;

    list_t  report_items;
    // Старое состояние
    uint8_t buttons[8];

    size_t                              report_buffer_pages;
    uintptr_t                           report_buffer_phys;
    void*                               report_buffer;
};

uint8_t hid_buttons_mappings[3] = {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE
};

void mouse_hid_report_handler(void* private_data, uint16_t usage_page, uint16_t usage_id, int64_t val) 
{
    struct usb_hid_mouse* mouse = private_data;
    struct mouse_event msevent;
    switch (usage_page)
    {
        case HID_USAGE_BUTTON:
            usage_id -= 1;
            uint8_t old_btn_state = mouse->buttons[usage_id];
            if (old_btn_state != val)
            {
                // usage_id - номер кнопки
                int new_state = ((old_btn_state == 0) && val == 1);
                uint8_t btnID = hid_buttons_mappings[usage_id];
                msevent.event_type = new_state == 1 ? MOUSE_EVENT_BUTTON_DOWN : MOUSE_EVENT_BUTTON_UP;
                msevent.u_event.btn.id = btnID;
                mouse_add_event(&msevent);
                mouse->buttons[usage_id] = val;
            }
            break;
        case HID_USAGE_PAGE_GENERIC:
            switch(usage_id)
            {
                case HID_USAGE_X:
                    if (val != 0)
                    {
                        msevent.event_type = MOUSE_EVENT_MOVE;
                        msevent.u_event.move.rel_x = val;
                        msevent.u_event.move.rel_y = 0;
                        mouse_add_event(&msevent);
                    }
                    break;
                case HID_USAGE_Y:
                    if (val != 0)
                    {
                        msevent.event_type = MOUSE_EVENT_MOVE;
                        msevent.u_event.move.rel_x = 0;
                        msevent.u_event.move.rel_y = val;
                        mouse_add_event(&msevent);
                    }
                    break;
                case HID_USAGE_WHEEL:
                    if (val != 0)
                    {
                        msevent.event_type = MOUSE_EVENT_SCROLL;
                        msevent.u_event.scroll.value = val;
                        mouse_add_event(&msevent);
                    }
                    break;
            }
            break;
    }
}

void mouse_event_callback(struct usb_msg* msg)
{
    struct usb_hid_mouse* mouse = msg->private;

    // Обработать отчет
    hid_process_report(mouse, mouse->report_buffer, msg->transferred_length, &mouse->report_items, mouse_hid_report_handler);

    // Послать запрос на прерывание
    usb_send_async_msg(mouse->device, mouse->ep, msg);
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
    int rc = usb_hid_set_protocol(device, interface, HID_PROTOCOL_REPORT);
    if (rc != 0)
    {
        printk("HID Mouse: SET_PROTOCOL failed (%i)\n", rc);
		return -1;
    }

    struct usb_hid_descriptor* hid_descriptor = interface->hid_descriptor;

    printk("HID Mouse. bcdHID %i, country %i descriptors %i\n", hid_descriptor->bcdHID, hid_descriptor->bCountryCode, hid_descriptor->bNumDescriptors);

    // Выделить объект мыши
    struct usb_hid_mouse* mouse = kmalloc(sizeof(struct usb_hid_mouse));
    memset(mouse, 0, sizeof(struct usb_hid_mouse));
    mouse->device = device;
    mouse->ep = interrupt_in_ep;

    uint32_t report_descr_index = 0;
    for (uint8_t i = 0; i < hid_descriptor->bNumDescriptors; i ++)
    {
        uint8_t descr_type = hid_descriptor->descriptors[i].bDescriptorType;
        uint16_t descr_len = hid_descriptor->descriptors[i].wDescriptorLength;

        printk("HID Mouse: Descr %i len %i\n", descr_type, descr_len);

        if (descr_type = HID_DESCRIPTOR_REPORT)
        {
            uint16_t wValue = (HID_DESCRIPTOR_REPORT << 8) | report_descr_index++;
            uint8_t *report_buffer = kmalloc(descr_len);

            struct usb_device_request req;
            req.type = USB_DEVICE_REQ_TYPE_STANDART;
            req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
            req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
            req.bRequest = USB_DEVICE_REQ_GET_DESCRIPTOR;
            req.wValue = wValue;
            req.wIndex = interface->descriptor.bInterfaceNumber;
            req.wLength = descr_len;

            // Получить report descriptor
            rc = usb_device_send_request(device, &req, report_buffer, descr_len);
            if (rc != 0)
            {
                printk("HID Mouse: GET_DESCRIPTOR failed (%i)\n", rc);
                return -1;
            }

            // Парсинг
            hid_parse_report(report_buffer, descr_len, &mouse->report_items);
            kfree(report_buffer);
            break;
        }
    }

    rc = usb_device_configure_endpoint(device, interrupt_in_ep);
	if (rc != 0)
	{
		printk("HID Mouse: Error configuring IN interrupt endpoint (%i)\n", rc);
		return -1;
	}

    // SET_IDLE
    usb_hid_set_idle(device, interface);

    // Выделить память под буфер report
    mouse->report_buffer_phys = (uintptr_t) pmm_alloc(max_packet_sz, &mouse->report_buffer_pages);
    mouse->report_buffer = map_io_region(mouse->report_buffer_phys, mouse->report_buffer_pages * PAGE_SIZE);

    struct usb_msg* msg = kmalloc(sizeof(struct usb_msg));
    msg->data = mouse->report_buffer_phys;
    msg->length = max_packet_sz;
    msg->callback = mouse_event_callback;
    msg->private = mouse;

    usb_send_async_msg(device, interrupt_in_ep, msg);

	return 0;
}

void usb_hid_mouse_device_remove(struct device *dev) 
{
	printk("HID Mouse: Device Remove\n");
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
