#include "dev/device_man.h"
#include "hid.h"
#include "kairax/stdio.h"
#include "mem/iomem.h"
#include "kairax/keycodes.h"
#include "kairax/string.h"
#include "drivers/char/input/keyboard.h"
#include "proc/tasklet.h"

#include "functions.h"

#define HID_FIRST_KEYCODE 4
#define HID_MAX_KEYCODE 0xE8

// Для зажигания светодиодов
#define HID_NUM_LOCK    0b001
#define HID_CAPS_LOCK   0b010
#define HID_SCROLL_LOCK 0b100
#define HID_COMPOSE     0b1000
#define HID_KANA        0b10000

uint8_t hid_kbd_bindings[256] = {
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
    [0x2E] = KRXK_EQUAL,
    [0x2F] = KRXK_LBRACE,
    [0x30] = KRXK_RBRACE,
    [0x31] = KRXK_BSLASH,

    [0x33] = KRXK_SEMICOLON,

    [0x35] = KRXK_BACK_TICK,

    [0x36] = KRXK_COMMA,
    [0x37] = KRXK_DOT,
    [0x38] = KRXK_SLASH,
    [0x39] = KRXK_CAPS,

    [0x47] = KRXK_SCRLOCK,

    [0x49] = KRXK_INSERT,
    [0x4A] = KRXK_HOME,
    [0x4b] = KRXK_PAGEUP,
    [0x4c] = KRXK_DEL,
    [0x4d] = KRXK_END,
    [0x4e] = KRXK_PAGEDOWN,
    [0x4F] = KRXK_RIGHT,
    [0x50] = KRXK_LEFT,
    [0x51] = KRXK_DOWN,
    [0x52] = KRXK_UP,
    [0x53] = KRXK_NUMLOCK,

    [0xE0] = KRXK_LCTRL,
    [0xE1] = KRXK_LSHIFT,
    [0xE2] = KRXK_LALT,
    [0xE3] = KRXK_LSUPER,
    [0xE4] = KRXK_RCTRL,
    [0xE5] = KRXK_RSHIFT,
    [0xE6] = KRXK_RALT,
    [0xE7] = KRXK_RSUPER,
};

struct usb_device_id usb_hid_kbd_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .bInterfaceClass = 3,
        .bInterfaceProtocol = 1 // Keyboard
    },
	{0,}
};

struct usb_hid_kbd {
    struct usb_device* device;
    struct usb_endpoint* ep;
    struct usb_interface* interface;

    // для приема отчета
    size_t       report_buffer_pages;
    uintptr_t    report_buffer_phys;
    uint8_t*     report_buffer;

    // для управления LED
    uint8_t*       led_ctrl;
    uintptr_t      led_ctrl_phys;
    struct usb_msg led_msg;
    int            upd_led;
    
    list_t  report_items;

    uint8_t keystat1[256];
    uint8_t keystat2[256];

    uint8_t *current;
    uint8_t *old;
};

void usb_hid_handle_led_key(struct usb_hid_kbd* kbd, uint8_t key);
void usb_hid_kbd_upd_led(struct usb_hid_kbd* kbd);
void hid_kbd_event_callback(struct usb_msg* msg);
void kbd_hid_report_handler(void* private_data, uint16_t usage_page, uint16_t usage_id, int64_t val);

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

void kbd_hid_report_handler(void* private_data, uint16_t usage_page, uint16_t usage_id, int64_t val) 
{
    struct usb_hid_kbd* kbd = private_data;

    if (usage_page != 0x07)
        return;

    uint8_t state = val > 0 ? 1 : 0;

    if (usage_id >= 4 && usage_id < 256)
		kbd->current[usage_id] = state;
}

void hid_kbd_event_callback(struct usb_msg* msg)
{
    struct usb_hid_kbd* kbd = msg->private;

    if (msg->status != 0)
    {
        printk("HID keyboard Error!!! (%i)\n", -msg->status);
    }

    // Обработать отчет
    hid_process_report(kbd, kbd->report_buffer, msg->transferred_length, &kbd->report_items, kbd_hid_report_handler);

    for (int i = HID_FIRST_KEYCODE; i < HID_MAX_KEYCODE; i ++)
    {
        uint8_t old = kbd->old[i];
        uint8_t current = kbd->current[i];
        uint8_t mapped = hid_kbd_bindings[i];

        if (old == 1 && current == 0)
        {
            keyboard_add_event(mapped, KRXK_RELEASED);
        }
        else if (old == 0 && current == 1)
        {
            usb_hid_handle_led_key(kbd, mapped);
            keyboard_add_event(mapped, KRXK_PRESSED);
        }

        kbd->old[i] = 0;
    }

    // поменяем массивы местами
    uint8_t* tmp = kbd->current;
    kbd->current = kbd->old;
    kbd->old = tmp;

    if (kbd->upd_led)
    {
        usb_hid_kbd_upd_led(kbd);
        kbd->upd_led = FALSE;
    }

    // Послать запрос на прерывание
    usb_send_async_msg(kbd->device, kbd->ep, msg);
}

void usb_hid_handle_led_key(struct usb_hid_kbd* kbd, uint8_t key)
{
    switch (key)
    {
        case KRXK_CAPS:
            *kbd->led_ctrl ^= HID_CAPS_LOCK;
            break;
        case KRXK_NUMLOCK:
            *kbd->led_ctrl ^= HID_NUM_LOCK;
            break;
        case KRXK_SCRLOCK:
            *kbd->led_ctrl ^= HID_SCROLL_LOCK;
            break;
        default:
            return;
    }

    kbd->upd_led = 1;
}

void usb_hid_kbd_upd_led(struct usb_hid_kbd* kbd)
{
    uint16_t value = (((uint16_t) 0x02) << 8) | 0;

    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = HID_SET_REPORT;
	req.wValue = value;
	req.wIndex = kbd->interface->descriptor.bInterfaceNumber;
	req.wLength = 1;

    struct usb_msg *msg = &kbd->led_msg;
    msg->data = (void*) kbd->led_ctrl_phys;
    msg->length = 1;
    msg->control_msg = &req;

    usb_device_send_async_request(kbd->device, msg);
}

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
    int rc = usb_hid_set_protocol(device, interface, HID_PROTOCOL_REPORT);
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

    struct usb_hid_descriptor* hid_descriptor = interface->hid_descriptor;

    struct usb_hid_kbd* kbd = kmalloc(sizeof(struct usb_hid_kbd));
    memset(kbd, 0, sizeof(struct usb_hid_kbd));
    kbd->device = device;
    kbd->interface = interface;
    kbd->ep = interrupt_in_ep;
    kbd->current = kbd->keystat1;
    kbd->old = kbd->keystat2;

    uint32_t report_descr_index = 0;
    for (uint8_t i = 0; i < hid_descriptor->bNumDescriptors; i ++)
    {
        uint8_t descr_type = hid_descriptor->descriptors[i].bDescriptorType;
        uint16_t descr_len = hid_descriptor->descriptors[i].wDescriptorLength;

        printk("HID KBD: Descr %i len %i\n", descr_type, descr_len);

        if (descr_type == HID_DESCRIPTOR_REPORT)
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
                printk("HID KBD: GET_DESCRIPTOR failed (%i)\n", rc);
                return -1;
            }

            // Парсинг
            hid_parse_report(report_buffer, descr_len, &kbd->report_items);
            kfree(report_buffer);
            break;
        }
    }

    // Выделить память под буфер report
    kbd->report_buffer_phys = (uintptr_t) pmm_alloc(max_packet_sz, &kbd->report_buffer_pages);
    kbd->report_buffer = (uint8_t*) map_io_region(kbd->report_buffer_phys, kbd->report_buffer_pages * PAGE_SIZE);

    // создаем объект usb_msg для получения отчетов в прерываниях
    struct usb_msg* msg = kmalloc(sizeof(struct usb_msg));
    msg->data = (void*) kbd->report_buffer_phys;
    msg->length = max_packet_sz;
    msg->callback = hid_kbd_event_callback;
    msg->private = kbd;

    // ожидание первого прерывания
    usb_send_async_msg(device, interrupt_in_ep, msg);

    // занулим msg для управления LED клавиатуры на всякий случай
    memset(&kbd->led_msg, 0, sizeof(struct usb_msg));

    // Вычислим адреса для управления LED
    // Используем память, выделенную под отчеты + размер пакета эндпоинта прерываний
    kbd->led_ctrl = kbd->report_buffer + max_packet_sz;
    kbd->led_ctrl_phys = kbd->report_buffer_phys + max_packet_sz;

    // Сначала выключаем все LED
    *kbd->led_ctrl = 0; 
    usb_hid_kbd_upd_led(kbd);

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

void usb_hid_kbd_init(void)
{
	register_usb_device_driver(&usb_hid_kbd_storage_driver);
}
