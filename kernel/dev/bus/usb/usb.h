#ifndef _USB_H
#define _USB_H

#include "kairax/types.h"
#include "usb_descriptors.h"

// Request Direction
#define USB_DEVICE_REQ_RECIPIENT_DEVICE	0
#define USB_DEVICE_REQ_RECIPIENT_INTERFACE	1
#define USB_DEVICE_REQ_RECIPIENT_ENDPOINT	2
#define USB_DEVICE_REQ_RECIPIENT_OTHER		3
#define USB_DEVICE_REQ_RECIPIENT_RESERVED	4
// request type
#define USB_DEVICE_REQ_TYPE_STANDART		0
#define USB_DEVICE_REQ_TYPE_CLASS			1
#define USB_DEVICE_REQ_TYPE_VENDOR			2
#define USB_DEVICE_REQ_TYPE_RSVD			3
// Request direction
#define USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE	0
#define USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST	1
// Requests
#define USB_DEVICE_REQ_GET_STATUS			0
#define USB_DEVICE_REQ_CLEAR_FEATURE		1
#define USB_DEVICE_REQ_SET_FEATURE			2
#define USB_DEVICE_REQ_SET_ADDRESS			5
#define USB_DEVICE_REQ_GET_DESCRIPTOR		6
#define USB_DEVICE_REQ_SET_DESCRIPTOR		7
#define USB_DEVICE_REQ_GET_CONFIGURATION	8
#define USB_DEVICE_REQ_SET_CONFIGURATION	9
#define USB_DEVICE_REQ_GET_INTERFACE		10
#define USB_DEVICE_REQ_SET_INTERFACE		11
#define USB_DEVICE_REQ_SYNC_FRAME			12
struct usb_device_request {
    
    uint8_t recipient           : 5;
    uint8_t type                : 2;
	uint8_t transfer_direction  : 1;

    uint8_t 	bRequest;
    uint16_t 	wValue;
    uint16_t 	wIndex;
    uint16_t 	wLength;
} PACKED;

// контроллеро-независимые представления
struct device;

struct usb_endpoint {
    struct usb_endpoint_descriptor descriptor;
    struct usb_ss_ep_companion_descriptor ss_companion;
    struct usb_ssp_isoc_ep_companion_descriptor ssp_isoc_companion;

    uint8_t ss_companion_present : 1;
    uint8_t ssp_isoc_companion_present : 1;

    void* transfer_ring;
};

struct usb_interface {
    struct usb_interface_descriptor descriptor;
    // Эндпоинты в количестве bNumEndpoints
    struct usb_endpoint* endpoints;
    // HID дескриптор (NULL, если отсутствует)
    struct usb_hid_descriptor* hid_descriptor;
    // Основное составное устройство
    struct device* root_device;
    // Устройство - интерфейс
    struct device* device;
};

struct usb_config {
    struct usb_configuration_descriptor descriptor;
    struct usb_interface**              interfaces;
};

struct usb_device {
    struct usb_device_descriptor    descriptor;
    struct usb_config**             configs;
    void*                           controller_device_data;

    char* product;
	char* manufacturer;
	char* serial;

    uint8_t slot_id;

    int (*send_request) (struct usb_device*, struct usb_device_request*, void*, uint32_t);
    int (*configure_endpoint) (struct usb_device*, struct usb_endpoint*);
};

// Создание и уничтожение
struct usb_device* new_usb_device(struct usb_device_descriptor* descriptor, void* controller_device_data);
void free_usb_device(struct usb_device* device);
// Общие функции управления

/// @brief 
/// @param device объект устройства
/// @param req указатель на структуру запроса
/// @param out указатель на буфер, куда читать
/// @param length длина выходных данных
/// @return 0 - при успехе
int usb_device_send_request(struct usb_device* device, struct usb_device_request* req, void* out, uint32_t length);
/// @brief сконфигурировать эндпоинт для работы с ним
/// @param device объект устройства
/// @param endpoint объект эндпоинта
/// @return 0 - при успехе
int usb_device_configure_endpoint(struct usb_device* device, struct usb_endpoint* endpoint);

struct usb_config* new_usb_config(struct usb_configuration_descriptor* descriptor);
void free_usb_config(struct usb_config* config);

struct usb_interface* new_usb_interface(struct usb_interface_descriptor* descriptor);
void free_usb_interface(struct usb_interface* iface);

struct usb_endpoint* new_usb_endpoint_array(size_t endpoints);

#endif