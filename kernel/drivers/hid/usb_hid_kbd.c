#include "dev/device_man.h"
#include "hid.h"
#include "kairax/stdio.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "mem/iomem.h"
#include "kairax/keycodes.h"
#include "kairax/string.h"
#include "drivers/char/input/keyboard.h"

#define HID_BOOT_MAX_KEYS   6

// Для зажигания светодиодов
#define HID_NUM_LOCK    0b1
#define HID_CAPS_LOCK   0b10
#define HID_SCROLL_LOCK 0b100
#define HID_COMPOSE     0b1000
#define HID_KANA        0b10000

short hid_kbd_bindings[256] = {
    [0x04] = KRXK_A,
    [0x05] = KRXK_B,
    [0x06] = KRXK_C,
    [0x07] = KRXK_D,
    [0x08] = KRXK_E,
    [0x09] = KRXK_F,
    [0x0A] = KRXK_G,
    [0x0B] = KRXK_H,
    [0x0C] = KRXK_I,
    [0x0D] = KRXK_J,
    [0x0E] = KRXK_K,
    [0x0F] = KRXK_L,
    [0x10] = KRXK_M,
    [0x11] = KRXK_N,
    [0x12] = KRXK_O,
    [0x13] = KRXK_P,
    [0x14] = KRXK_Q,
    [0x15] = KRXK_R,
    [0x16] = KRXK_S,
    [0x17] = KRXK_T,
    [0x18] = KRXK_U,
    [0x19] = KRXK_V,
    [0x1A] = KRXK_W,
    [0x1B] = KRXK_X,
    [0x1C] = KRXK_Y,
    [0x1D] = KRXK_Z,
    // Верхние цифры
    [0x1E] = KRXK_1,
    [0x1F] = KRXK_2,
    [0x20] = KRXK_3,
    [0x21] = KRXK_4,
    [0x22] = KRXK_5,
    [0x23] = KRXK_6,
    [0x24] = KRXK_7,
    [0x25] = KRXK_8,
    [0x26] = KRXK_9,
    [0x27] = KRXK_0,
    // F1 - F12
    [0x3A] = KRXK_F1,
    [0x3B] = KRXK_F2,
    [0x3C] = KRXK_F3,
    [0x3D] = KRXK_F4,
    [0x3E] = KRXK_F5,
    [0x3F] = KRXK_F6,
    [0x40] = KRXK_F7,
    [0x41] = KRXK_F8,
    [0x42] = KRXK_F9,
    [0x43] = KRXK_F10,
    [0x44] = KRXK_F11,
    [0x45] = KRXK_F12,
    
    [0x28] = KRXK_ENTER,
    [0x29] = KRXK_ESCAPE,
    [0x2A] = KRXK_BKSP,
    [0x2B] = KRXK_TAB,
    [0x2C] = KRXK_SPACE,
    [0x2D] = KRXK_MINUS,
    [0x2E] = KRXK_PLUS,

    [0x31] = KRXK_BSLASH,

    [0x33] = KRXK_SEMICOLON,

    [0x36] = KRXK_COMMA,
    [0x37] = KRXK_DOT,
    [0x38] = KRXK_SLASH,
    [0x39] = KRXK_CAPS
};

struct usb_device_id usb_hid_kbd_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .bInterfaceClass = 3,
        .bInterfaceSubclass = 1, // Supports Boot protocol
        .bInterfaceProtocol = 1 // Keyboard
    },
	{0,}
};

struct hid_kbd_boot_int_report 
{
    uint8_t modifier_key_status;
    uint8_t reserved;
    uint8_t presses[HID_BOOT_MAX_KEYS];
};

struct usb_hid_kbd {
    struct usb_device* device;
    struct usb_endpoint* ep;

    size_t                              report_buffer_pages;
    uintptr_t                           report_buffer_phys;
    struct hid_kbd_boot_int_report*     report_buffer;
    // Буфер для хранения предыдущего состояния
    uint8_t                             old_state[HID_BOOT_MAX_KEYS];
};

int usb_hid_set_report(struct usb_device* device, struct usb_interface* interface, uint8_t report_type, uint8_t report_id, void* report, uint32_t report_length)
{
    uint16_t value = (((uint16_t) report_type) << 8) | report_id;

    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = HID_SET_REPORT;
	req.wValue = value;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = report_length;

    return usb_device_send_request(device, &req, report, report_length);
}

void event_callback(struct usb_msg* msg)
{
    struct usb_hid_kbd* kbd = msg->private;
    struct hid_kbd_boot_int_report* rep = kbd->report_buffer;
    uint16_t i, j;
    uint8_t mapped;
    int found = FALSE;

    //printk("Key pressed %x %x %x %x %x %x\n", rep->presses[0], rep->presses[1], rep->presses[2], rep->presses[3], rep->presses[4], rep->presses[5]);

    // Обработать нажатия
    for (i = 0; i < HID_BOOT_MAX_KEYS; i ++)
    {
        uint8_t ek = rep->presses[i];
        if (ek == 0) continue;

        found = FALSE;

        for (j = 0; j < HID_BOOT_MAX_KEYS; j ++)
        {
            if (kbd->old_state[j] == ek)
            {
                // Кнопка уже была зажата в прошлом сообщении
                found = TRUE;
                break;
            }
        }

        if (found == FALSE)
        {
            mapped = hid_kbd_bindings[ek];
            keyboard_add_event(mapped, KRXK_PRESSED);
        }
    }

    // Обработать отпускания клавиш
    for (i = 0; i < HID_BOOT_MAX_KEYS; i ++)
    {
        uint8_t ek = kbd->old_state[i];
        if (ek == 0) continue;

        found = FALSE;
        
        for (j = 0; j < HID_BOOT_MAX_KEYS; j ++)
        {
            if (rep->presses[j] == ek)
            {
                // Кнопка по прежнему зажата
                found = TRUE;
                break;
            }
        }

        if (found == FALSE)
        {
            mapped = hid_kbd_bindings[ek];
            keyboard_add_event(mapped, KRXK_RELEASED);
        }
    }

    // Обновить буфер
    memcpy(kbd->old_state, rep->presses, HID_BOOT_MAX_KEYS);

    // Ожидание прерывания
    usb_send_async_msg(kbd->device, kbd->ep, msg);
}

struct usb_endpoint* usb_hid_find_ep(struct usb_interface* interface)
{
    for (uint8_t i = 0; i < interface->descriptor.bNumEndpoints; i ++)
	{
		struct usb_endpoint* ep = &interface->endpoints[i];

		// Нам интересны только Interrupt
		if ((ep->descriptor.bmAttributes & USB_ENDPOINT_ATTR_TT_MASK) != USB_ENDPOINT_ATTR_TT_INTERRUPT)
			continue;

        // и только IN
		if ((ep->descriptor.bEndpointAddress & USB_ENDPOINT_ADDR_DIRECTION_IN) == USB_ENDPOINT_ADDR_DIRECTION_IN)
		{
			return ep;
		}
	}

    return NULL;
}

/*
int usb_hid_kbd_init_report_prot()
{
    struct usb_hid_descriptor* hid_descriptor = interface->hid_descriptor;

    printk("HID keyboard. bcdHID %i, country %i descriptors %i\n", hid_descriptor->bcdHID, hid_descriptor->bCountryCode, hid_descriptor->bNumDescriptors);

    uint16_t hid_descr_length = 0;
    uint16_t report_descr_length = 0;
    // Получить размеры дескрипторов
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
}*/

int usb_hid_kbd_device_probe(struct device *dev) 
{   
    struct usb_interface* interface = dev->usb_info.usb_interface;
    struct usb_device* device = dev->usb_info.usb_device;

    // Эндпоинт
    struct usb_endpoint* interrupt_in_ep = NULL;
    size_t max_packet_sz = 0;

    // Поиск необходимого эндпоинта
    interrupt_in_ep = usb_hid_find_ep(interface);
    // Не найдены необходимые endpoints
	if (interrupt_in_ep == NULL)
	{
        printk("HID: No IN Interrupt endpoint\n");
		return -1;
	}

    // Размер пакета
    max_packet_sz = interrupt_in_ep->descriptor.wMaxPacketSize & 0x07FF;
    printk("HID: interrupt ep max pack size %i\n", interrupt_in_ep->descriptor.wMaxPacketSize);

    // Пока используем Boot
    int rc = usb_hid_set_protocol(device, interface, HID_PROTOCOL_BOOT);
    if (rc != 0)
    {
        printk("HID: SET_PROTOCOL failed (%i)\n", rc);
		return -1;
    }

    rc = usb_device_configure_endpoint(device, interrupt_in_ep);
	if (rc != 0)
	{
		printk("HID KBD: Error configuring IN interrupt endpoint (%i)\n", rc);
		return -1;
	}

    // SET_IDLE
    usb_hid_set_idle(device, interface);

    // GET_REPORT
    uint8_t report[8];
    rc = usb_hid_get_report(device, interface, 0x01, 0, report, 8);
    if (rc != 0)
    {
        printk("HID: GET_REPORT failed (%i)\n", rc);
		return -1;
    }

    struct usb_hid_kbd* kbd = kmalloc(sizeof(struct usb_hid_kbd));
    memset(kbd, 0, sizeof(struct usb_hid_kbd));
    kbd->device = device;
    kbd->ep = interrupt_in_ep;

    // Выделить память под буфер report
    kbd->report_buffer_phys = (uintptr_t) pmm_alloc(max_packet_sz, &kbd->report_buffer_pages);
    kbd->report_buffer = map_io_region(kbd->report_buffer_phys, kbd->report_buffer_pages * PAGE_SIZE);

    struct usb_msg* msg = kmalloc(sizeof(struct usb_msg));
    msg->data = kbd->report_buffer_phys;
    msg->length = max_packet_sz;
    msg->callback = event_callback;
    msg->private = kbd;

    usb_send_async_msg(device, interrupt_in_ep, msg);

    // Сбросим LED индикаторы клавиатуры
    uint8_t val = 0;
    rc = usb_hid_set_report(device, interface, 0x02, 0, &val, 1);
    
	return 0;
}

void usb_hid_kbd_device_remove(struct device *dev) 
{
	printk("HID KBD: Device Remove\n");
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
