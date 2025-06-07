#include "usb.h"
#include "mem/kheap.h"
#include "string.h"

struct usb_device* new_usb_device(struct usb_device_descriptor* descriptor, void* controller_device_data)
{
    struct usb_device* result = kmalloc(sizeof(struct usb_device));
    memcpy(&result->descriptor, descriptor, sizeof(struct usb_device_descriptor));
    result->controller_device_data = controller_device_data;
    result->configs = kmalloc(descriptor->bNumConfigurations * sizeof(struct usb_config*));

    return result;
}

void free_usb_device(struct usb_device* device)
{
    // TODO: implement
    KFREE_SAFE(device->product)
    KFREE_SAFE(device->manufacturer)
    KFREE_SAFE(device->serial)

    for (size_t i = 0; i < device->descriptor.bNumConfigurations; i ++)
    {
        free_usb_config(device->configs[i]);
    }
    kfree(device->configs);

    kfree(device);
}

int usb_device_send_request(struct usb_device* device, struct usb_device_request* req, void* out, uint32_t length)
{
    return device->send_request(device, req, out, length);
}

int usb_device_configure_endpoint(struct usb_device* device, struct usb_endpoint* endpoint)
{
    return device->configure_endpoint(device, endpoint);
}

struct usb_config* new_usb_config(struct usb_configuration_descriptor* descriptor)
{
    struct usb_config* result = kmalloc(sizeof(struct usb_config));
    memcpy(&result->descriptor, descriptor, sizeof(struct usb_configuration_descriptor));
    result->interfaces = kmalloc(descriptor->bNumInterfaces * sizeof(struct usb_interface*));

    return result;
}

void free_usb_config(struct usb_config* config)
{
    // TODO: implement
    for (size_t i = 0; i < config->descriptor.bNumInterfaces; i ++)
    {
        free_usb_interface(config->interfaces[i]);
    }
    kfree(config->interfaces);

    kfree(config);
}

struct usb_interface* new_usb_interface(struct usb_interface_descriptor* descriptor)
{
    struct usb_interface* result = kmalloc(sizeof(struct usb_interface));
    memset(result, 0, sizeof(struct usb_interface));

    // Записать информацию о дескрипторе
    memcpy(&result->descriptor, descriptor, sizeof(struct usb_interface_descriptor));
    
    // Выделить память под массив эндпоинтов
    result->endpoints = new_usb_endpoint_array(descriptor->bNumEndpoints);

    return result;
}

void free_usb_interface(struct usb_interface* iface)
{
    // TODO: implement
    KFREE_SAFE(iface->hid_descriptor)
    KFREE_SAFE(iface->endpoints);
    kfree(iface);
}

struct usb_endpoint* new_usb_endpoint_array(size_t endpoints)
{
    size_t memsz = sizeof(struct usb_endpoint) * endpoints;

    struct usb_endpoint* result = kmalloc(memsz);
    memset(result, 0, memsz);

    return result;
}