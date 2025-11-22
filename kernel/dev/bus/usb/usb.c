#include "usb.h"
#include "mem/kheap.h"
#include "string.h"
#include "kairax/stdio.h"
#include "kairax/errors.h"

extern struct pci_device_driver ehci_ctrl_driver;
extern struct pci_device_driver xhci_ctrl_driver;

void usb_init()
{
    register_pci_device_driver(&ehci_ctrl_driver);
	register_pci_device_driver(&xhci_ctrl_driver);
}

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

struct usb_msg *new_usb_msg()
{
    struct usb_msg *msg = kmalloc(sizeof(struct usb_msg));
    if (msg == NULL)
        return NULL;
	memset(msg, 0, sizeof(struct usb_msg));
    return msg;
}

int usb_device_send_request(struct usb_device* device, struct usb_device_request* req, void* out, uint32_t length)
{
    if (device->state == USB_STATE_DISCONNECTED)
        return -ENODEV;
    return device->send_request(device, req, out, length);
}

int usb_device_configure_endpoint(struct usb_device* device, struct usb_endpoint* endpoint)
{
    if (device->state == USB_STATE_DISCONNECTED)
        return -ENODEV;
    return device->configure_endpoint(device, endpoint);
}

int usb_device_bulk_msg(struct usb_device* device, struct usb_endpoint* endpoint, void* data, uint32_t length)
{
    if (device->state == USB_STATE_DISCONNECTED)
        return -ENODEV;
	return device->bulk_msg(device, endpoint, data, length);
}

int usb_send_async_msg(struct usb_device* device, struct usb_endpoint* endpoint, struct usb_msg *msg)
{
    if (device->state == USB_STATE_DISCONNECTED)
        return -ENODEV;
    return device->async_msg(device, endpoint, msg);
}

ssize_t usb_get_string(struct usb_device* device, int index, char* buffer, size_t buflen)
{
    if (device->state == USB_STATE_DISCONNECTED)
        return -ENODEV;
    return device->get_string(device, index, buffer, buflen);
}

int usb_set_interface(struct usb_device* device, int interfaceNumber, int altsetting)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = USB_DEVICE_REQ_SET_INTERFACE;
	req.wValue = altsetting;
	req.wIndex = interfaceNumber;
	req.wLength = 0;

    return usb_device_send_request(device, &req, NULL, 0);
}

struct usb_config* new_usb_config(struct usb_configuration_descriptor* descriptor)
{
    struct usb_config* result = kmalloc(sizeof(struct usb_config));
    result->interfaces_num = 0;
    // Копируем дескриптор
    memcpy(&result->descriptor, descriptor, sizeof(struct usb_configuration_descriptor));
    // Выделить память под массив указателей на интерфейсы
    size_t interfaces_mem_sz = descriptor->bNumInterfaces * sizeof(struct usb_interface*);
    result->interfaces = kmalloc(interfaces_mem_sz);
    memset(result->interfaces, 0, interfaces_mem_sz);

    return result;
}

void usb_config_put_interface(struct usb_config* config, struct usb_interface* iface)
{
    if (config->interfaces_num >= config->descriptor.bNumInterfaces)
    {
        // В буфере оказалось больше интерфейсов, чем указано в конфигурации
        // Такая ситуация бывает у некоторых устройств, придется обрабатывать
        printk("USB: Warn - bNumInterfaces is less than actual\n");

        // Выделим новую память
        struct usb_interface** newbuffer = kmalloc((config->interfaces_num + 1) * sizeof(struct usb_interface*));
        // Скопируем старые
        memcpy(newbuffer, config->interfaces, config->interfaces_num * sizeof(struct usb_interface*));
        // Освободим старую память
        kfree(config->interfaces);
        config->interfaces = newbuffer;
    }

    config->interfaces[config->interfaces_num++] = iface;
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

void usb_interface_add_descriptor(struct usb_interface* iface, struct usb_descriptor_header* descriptor_header)
{
    // Перевыделим память
    struct usb_descriptor_header **newbuffer = kmalloc((iface->other_descriptors_num + 1) * sizeof(struct usb_descriptor_header*));
    // Если массив уже был - скопируем имеющиеся адреса и очистим старый
    if (iface->other_descriptors_num > 0)
    {
        memcpy(newbuffer, iface->other_descriptors, iface->other_descriptors_num * sizeof(struct usb_descriptor_header*));
        kfree(iface->other_descriptors);
    }

    // Делаем копию дескриптора
    struct usb_descriptor_header* copy_descriptor_header = kmalloc(descriptor_header->bLength);
    memcpy(copy_descriptor_header, descriptor_header, descriptor_header->bLength);

    // Сохраняем указатель на копию
    iface->other_descriptors = newbuffer;
    iface->other_descriptors[iface->other_descriptors_num++] = copy_descriptor_header;
}

void free_usb_interface(struct usb_interface* iface)
{
    // TODO: implement
    KFREE_SAFE(iface->hid_descriptor)
    KFREE_SAFE(iface->endpoints);

    for (size_t i = 0; i < iface->other_descriptors_num; i ++)
    {
        KFREE_SAFE(iface->other_descriptors[i]);
    }
    KFREE_SAFE(iface->other_descriptors);

    kfree(iface);
}

struct usb_endpoint* new_usb_endpoint_array(size_t endpoints)
{
    size_t memsz = sizeof(struct usb_endpoint) * endpoints;

    struct usb_endpoint* result = kmalloc(memsz);
    memset(result, 0, memsz);

    return result;
}