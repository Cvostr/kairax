#include "usb.h"
#include "mem/kheap.h"
#include "string.h"

struct usb_interface* new_usb_interface(struct usb_interface_descriptor* descriptor)
{
    struct usb_interface* result = kmalloc(sizeof(struct usb_interface));
    memset(result, 0, sizeof(struct usb_interface));

    // Записать информацию о дескрипторе
    memcpy(&result->descriptor, descriptor, sizeof(struct usb_interface));

    return result;
}

struct usb_endpoint* new_usb_endpoint_array(size_t endpoints)
{
    size_t memsz = sizeof(struct usb_endpoint) * endpoints;

    struct usb_endpoint* result = kmalloc(memsz);
    memset(result, 0, memsz);

    return result;
}