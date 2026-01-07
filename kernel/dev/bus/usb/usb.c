#include "usb.h"
#include "mem/kheap.h"
#include "string.h"
#include "kairax/stdio.h"
#include "kairax/errors.h"
#include "kairax/kstdlib.h"

extern struct pci_device_driver ehci_ctrl_driver;
extern struct pci_device_driver xhci_ctrl_driver;

//#define USB_LOG_BINTERFACE_NUM_PROBLEM

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

int usb_device_send_async_request(struct usb_device* device, struct usb_msg *msg)
{
    if (device->state == USB_STATE_DISCONNECTED)
        return -ENODEV;
    if (msg->status == -EINPROGRESS)
        return -ERROR_BUSY;
    return device->send_async_request(device, msg);
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
    if (msg->status == -EINPROGRESS)
        return -ERROR_BUSY;
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

int usb_clear_endpoint_halt(struct usb_device* device, struct usb_endpoint* ep)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_ENDPOINT;
	req.bRequest = USB_DEVICE_REQ_CLEAR_FEATURE;
	req.wValue = USB_ENDPOINT_HALT;
	req.wIndex = ep->descriptor.bEndpointAddress;
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

int usb_device_set_configuration(struct usb_device* device, int configuration)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = USB_DEVICE_REQ_SET_CONFIGURATION;
	req.wValue = configuration;
	req.wIndex = 0;
	req.wLength = 0; // Данный вид запроса не имеет выходных данных

    return usb_device_send_request(device, &req, NULL, 0);
}

int usb_device_get_configuration_descriptor(struct usb_device* device, uint8_t configuration, struct usb_configuration_descriptor** descr)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = USB_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DESCRIPTOR_TYPE_CONFIGURATION << 8) | configuration;
	req.wIndex = 0;
	req.wLength = sizeof(struct usb_descriptor_header);
    
    // Временный объект
    struct usb_configuration_descriptor temp_descriptor;

    // Считываем только заголовок, чтобы узнать реальный размер дескриптора
    int rc = usb_device_send_request(device, &req, &temp_descriptor, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    // Считываем дескриптор без данных чтобы узнать его реальный размер
    req.wLength = temp_descriptor.header.bLength;
    rc = usb_device_send_request(device, &req, &temp_descriptor, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    // Считываем дескриптор со всеми данными
    *descr = kmalloc(temp_descriptor.wTotalLength);
    req.wLength = temp_descriptor.wTotalLength;
    rc = usb_device_send_request(device, &req, *descr, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}

int usb_get_string_descriptor(struct usb_device* device, uint8_t index, uint16_t language_id, struct usb_string_descriptor *out, size_t len)
{
    int rc;
    struct usb_device_request req;
    struct usb_descriptor_header header;
    
    // заполним начальный запрос
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = USB_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DESCRIPTOR_TYPE_STRING << 8) | index;
	req.wIndex = language_id;
	req.wLength = sizeof(struct usb_descriptor_header);

    // Считываем только заголовок, чтобы узнать реальный размер дескриптора
    rc = usb_device_send_request(device, &req, &header, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    // Размер считываемых данных
    size_t read_length = MIN(header.bLength, len);

    req.wLength = read_length;
    // Считываем дескриптор
    rc = usb_device_send_request(device, &req, out, read_length);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}

struct usb_config* usb_parse_configuration_descriptor(struct usb_configuration_descriptor* config_descriptor)
{
    // Создаем струтуру конфигурации
    struct usb_config* usb_conf = new_usb_config(config_descriptor);

	// Парсим конфигурацию
	uint8_t* conf_buffer = config_descriptor->data;
	size_t offset = 0;
	size_t len = config_descriptor->wTotalLength - config_descriptor->header.bLength;

    // Указатель на текущий обрабатываемый интерфейс
    struct usb_interface* current_interface = NULL;
    // Количество прочитанных эндпоинтов у текущего интерфейса
    size_t processed_endpoints = 0;
    // Указатель на текущий обрабатываемый эндпоинт
    struct usb_endpoint* current_endpoint = NULL;

	while (offset < len)
	{
        // Заголовок
		struct usb_descriptor_header* config_header = (struct usb_descriptor_header*) &(conf_buffer[offset]);

		switch (config_header->bDescriptorType)
		{
			case USB_DESCRIPTOR_INTERFACE:
				struct usb_interface_descriptor* iface_descr = (struct usb_interface_descriptor*) config_header; 
                printk("USB: interface class %i subcl %i prot %i with %i endpoints\n", iface_descr->bInterfaceClass, iface_descr->bInterfaceSubClass, iface_descr->bInterfaceProtocol, iface_descr->bNumEndpoints);

                // Создать общий объект интерфейса
                current_interface = new_usb_interface(iface_descr);
                current_interface->parent_config = usb_conf;

                // Сбросить счетчик эндпоинтов 
                processed_endpoints = 0;
                current_endpoint = NULL;

                // Добавить в массив структуры конфигурации
                usb_config_put_interface(usb_conf, current_interface);
                break;
			case USB_DESCRIPTOR_ENDPOINT:
                if (current_interface != NULL)
                {
                    // Скопировать информацию об endpoint
                    struct usb_endpoint* endpoint = &current_interface->endpoints[processed_endpoints++];
                    memcpy(&endpoint->descriptor, config_header, sizeof(struct usb_endpoint_descriptor));
                    current_endpoint = endpoint;
                } 
                else 
                {
                    printk("USB: No interface descriptor before this endpoint descriptor");
                }
				break;
            case USB_DESCRIPTOR_SUPERSPEED_ENDPOINT_COMPANION:
                // Записать дескриптор
                if (current_endpoint != NULL)
                {
                    memcpy(&current_endpoint->ss_companion, config_header, sizeof(struct usb_ss_ep_companion_descriptor));
                    current_endpoint->ss_companion_present = 1;
                }
                else
                {
                    printk("USB: No endpoint descriptor before this super speed companion descriptor");
                }
                break;
            case USB_DESCRIPTOR_SUPERSPEEDPLUS_ISO_ENDPOINT_COMPANION:
                printk("USB: USB_DESCRIPTOR_SUPERSPEEDPLUS_ISO_ENDPOINT_COMPANION\n");
                // Записать дескриптор
                if (current_endpoint != NULL)
                {
                    memcpy(&current_endpoint->ssp_isoc_companion, config_header, sizeof(struct usb_ssp_isoc_ep_companion_descriptor));
                    current_endpoint->ssp_isoc_companion_present = 1;
                }
                break;
			case USB_DESCRIPTOR_HID:
                current_interface->hid_descriptor = kmalloc(config_header->bLength);
                memcpy(current_interface->hid_descriptor, config_header, config_header->bLength);
				break;
			default:
				printk("USB: unknown device config %i\n", config_header->bDescriptorType);
                // Неизвестный тип - сохраняем в отдельный массив
                if (current_interface != NULL)
                {
                    usb_interface_add_descriptor(current_interface, config_header);
                }
                break;
		}

		offset += config_header->bLength;
	}

    return usb_conf;
}

void usb_config_put_interface(struct usb_config* config, struct usb_interface* iface)
{
    if (config->interfaces_num >= config->descriptor.bNumInterfaces)
    {
        // В буфере оказалось больше интерфейсов, чем указано в конфигурации
        // Такая ситуация бывает у некоторых устройств, придется обрабатывать
#ifdef USB_LOG_BINTERFACE_NUM_PROBLEM
        printk("USB: Warn - bNumInterfaces is less than actual\n");
#endif

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