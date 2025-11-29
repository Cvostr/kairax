#ifndef _HID_H
#define _HID_H

#include "dev/bus/usb/usb.h"
#include "kairax/list/list.h"

#define HID_SET_REPORT   0x09
#define HID_SET_IDLE     0x0A
#define HID_SET_PROTOCOL 0x0B

#define HID_PROTOCOL_BOOT   0
#define HID_PROTOCOL_REPORT 1

#define HID_DESCRIPTOR_HID 0x21
#define HID_DESCRIPTOR_REPORT 0x22
#define HID_DESCRIPTOR_PHYSICAL 0x23

// USAGE PAGES
#define HID_USAGE_PAGE_GENERIC          0x1
#define HID_USAGE_PAGE_GAME_CONTROLS    0x5
#define HID_USAGE_PAGE_LED              0x8
#define HID_USAGE_BUTTON                0x9

// USAGE_IDs
#define HID_USAGE_POINTER       0x1
#define HID_USAGE_MOUSE         0x2
#define HID_USAGE_JOYSTICK      0x4
#define HID_USAGE_GAMEPAD       0x5
#define HID_USAGE_KEYBOARD      0x6
#define HID_USAGE_KEYPAD        0x7
#define HID_USAGE_X             0x30
#define HID_USAGE_Y             0x31
#define HID_USAGE_Z             0x32
#define HID_USAGE_WHEEL         0x38

#define HID_REPORT_ITEM_TYPE_MAIN   0
#define HID_REPORT_ITEM_TYPE_GLOBAL 1
#define HID_REPORT_ITEM_TYPE_LOCAL  2
#define HID_REPORT_ITEM_TYPE_RESERVED 3

// Флаги для Input, Output, Feature
#define HID_ITEM_FLAG_CONSTANT  (1 << 0)
#define HID_ITEM_FLAG_VARIABLE  (1 << 1)
#define HID_ITEM_FLAG_RELATIVE  (1 << 2)
#define HID_ITEM_FLAG_WRAP      (1 << 3)
#define HID_ITEM_FLAG_NON_LINEAR    (1 << 4)
#define HID_ITEM_FLAG_NO_PREFERRED_STATE (1 << 5)
#define HID_ITEM_FLAG_NULLSTATE (1 << 6)
#define HID_ITEM_FLAG_VOLATILE  (1 << 7)
#define HID_ITEM_FLAG_BUFFERED_BYTES (1 << 8)

typedef struct {
    union {
        uint8_t u8;
        int8_t i8;
        uint16_t u16;
        int16_t i16;
        uint32_t u32;
        int32_t i32;
    } v;
    uint8_t size;
} hid_value_t;

enum hid_report_item_type {
    INPUT, OUTPUT, FEATURE
};

struct hid_report_item {
    enum hid_report_item_type type;
    uint16_t usage_page;
    uint16_t usage_id;

    uint32_t usage_minimum;
    uint32_t usage_maximum;

    uint8_t report_id;
    uint32_t report_size;
    uint32_t report_count;

    int32_t logical_minimum;
    int64_t logical_maximum;

    uint32_t value;
};

void hid_report_read(uint8_t *report, size_t pos, uint8_t bSize, hid_value_t *val);
int32_t hid_get_signed_value(hid_value_t *val);
uint16_t hid_report_read16u(uint8_t *report, size_t pos, uint8_t bSize);
int32_t hid_report_read32(uint8_t *report, size_t pos, uint8_t bSize);
uint32_t hid_report_read32u(uint8_t *report, size_t pos, uint8_t bSize);
void hid_parse_report(uint8_t* report, size_t length, list_t *items);

int usb_hid_set_idle(struct usb_device* device, struct usb_interface* interface);
int usb_hid_get_report(struct usb_device* device, struct usb_interface* interface, uint8_t report_type, uint8_t report_id, void* report, uint32_t report_length);
int usb_hid_set_protocol(struct usb_device* device, struct usb_interface* interface, int protocol);

typedef void (*hid_data_handle_routine) (void* private_data, uint16_t usage_page, uint16_t usage_id, int64_t val);
void hid_process_report(void* private_data, uint8_t *report, size_t len, list_t *items, hid_data_handle_routine handler);

#endif