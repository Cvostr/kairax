#include "usb_xhci.h"
#include "usb_xhci_contexts.h"
#include "mem/kheap.h"
#include "string.h"
#include "mem/iomem.h"
#include "mem/pmm.h"
#include "kairax/kstdlib.h"
#include "../usb.h"
#include "dev/device.h"
#include "dev/device_man.h"

#define XHCI_TRANSFER_RING_ENTITIES 256
#define XHCI_CONTROL_EP_AVG_TRB_LEN 8

//#define XHCI_LOG_TRANSFER
#define XHCI_LOG_EP_CONFIGURE

struct xhci_device* new_xhci_device(struct xhci_controller* controller, uint8_t port_id, uint8_t slot_id)
{
    struct xhci_device* dev = kmalloc(sizeof(struct xhci_device));
	memset(dev, 0, sizeof(struct xhci_device));

    dev->controller = controller;
    dev->port_id = port_id;
    dev->slot_id = slot_id;
    dev->ctx_size = controller->context_size;

    return dev;
}

void xhci_free_device(struct xhci_device* dev)
{
    xhci_free_transfer_ring(dev->control_transfer_ring);

    // free contexts
    unmap_io_region(dev->input_ctx, dev->input_ctx_pages * PAGE_SIZE);
    pmm_free_pages(dev->input_ctx_phys, dev->input_ctx_pages);

    unmap_io_region(dev->device_ctx, dev->device_ctx_pages * PAGE_SIZE);
    pmm_free_pages(dev->device_ctx_phys, dev->device_ctx_pages);

    // TODO: use refcount
    kfree(dev);
}

int xhci_device_init_contexts(struct xhci_device* dev)
{
    size_t input_context_sz = dev->ctx_size == 1 ? sizeof(struct xhci_input_context64) : sizeof(struct xhci_input_context32);   
    size_t device_context_sz = dev->ctx_size == 1 ? sizeof(struct xhci_device_context64) : sizeof(struct xhci_device_context32);   

    // Выделить память под Input Context
    dev->input_ctx_phys = (uintptr_t) pmm_alloc(input_context_sz, &dev->input_ctx_pages);
	dev->input_ctx = map_io_region(dev->input_ctx_phys, input_context_sz);
    memset(dev->input_ctx, 0, input_context_sz);

    // Выделить память под Device Output Context (управляется устройством)
    dev->device_ctx_phys = (uintptr_t) pmm_alloc(device_context_sz, &dev->device_ctx_pages);
	dev->device_ctx = map_io_region(dev->device_ctx_phys, device_context_sz);
    memset(dev->device_ctx, 0, device_context_sz);

    if (dev->ctx_size == 1)
    {
        // 64 битный контекст
        struct xhci_input_context64* input_ctx = dev->input_ctx;
        dev->input_control_context = &input_ctx->input_ctrl_ctx;
        dev->slot_ctx = &input_ctx->device_ctx.slot_context;
        dev->control_endpoint_ctx = &input_ctx->device_ctx.control_ep_context;
    }
    else
    {
        // 32 битный контекст
        struct xhci_input_context32* input_ctx = dev->input_ctx;
        dev->input_control_context = &input_ctx->input_ctrl_ctx;
        dev->slot_ctx = &input_ctx->device_ctx.slot_context;
        dev->control_endpoint_ctx = &input_ctx->device_ctx.control_ep_context;
    }

    dev->control_transfer_ring = xhci_create_transfer_ring(XHCI_TRANSFER_RING_ENTITIES);

    return 0;
}

void xhci_device_configure_control_endpoint_ctx(struct xhci_device* dev, uint16_t max_packet_size)
{
    struct xhci_input_control_context32* input_control_context = dev->input_control_context;
    input_control_context->add_flags = (1 << 1) | (1 << 0);
    input_control_context->drop_flags = 0;

    struct xhci_slot_context32* slot_ctx = dev->slot_ctx;
    slot_ctx->context_entries = 1;
    slot_ctx->speed = dev->port_speed;
    slot_ctx->root_hub_port_num = dev->port_id + 1; // приводим к 1 - ...
    slot_ctx->route_string = 0;
    slot_ctx->interrupter_target = 0;

    struct xhci_endpoint_context32* ctrl_endpoint_ctx = dev->control_endpoint_ctx;
    ctrl_endpoint_ctx->endpoint_state = XHCI_ENDPOINT_STATE_DISABLED;
    ctrl_endpoint_ctx->endpoint_type = XHCI_ENDPOINT_TYPE_CONTROL;
    ctrl_endpoint_ctx->interval = 0;
    ctrl_endpoint_ctx->error_count = 3;
    ctrl_endpoint_ctx->max_packet_size = max_packet_size;
    ctrl_endpoint_ctx->transfer_ring_dequeue_ptr = xhci_transfer_ring_get_cur_phys_ptr(dev->control_transfer_ring);
    ctrl_endpoint_ctx->dcs = dev->control_transfer_ring->cycle_bit;
    ctrl_endpoint_ctx->max_esit_payload_lo = 0;
    ctrl_endpoint_ctx->max_esit_payload_hi = 0;
    ctrl_endpoint_ctx->average_trb_length = XHCI_CONTROL_EP_AVG_TRB_LEN;
}

int xhci_device_update_actual_max_packet_size(struct xhci_device* dev, uint32_t max_packet_size)
{
    // Обновляем значение
    struct xhci_endpoint_context32* ctrl_endpoint_ctx = dev->control_endpoint_ctx;
    ctrl_endpoint_ctx->max_packet_size = max_packet_size;

    // Обновляется Control Endpoint (с индексом 1)
    struct xhci_input_control_context32* input_control_context = dev->input_control_context;
    input_control_context->add_flags = (1 << 1);

    return xhci_controller_device_eval_ctx(dev->controller, dev->input_ctx_phys, dev->slot_id);
}

int xhci_device_handle_transfer_event(struct xhci_device* dev, struct xhci_trb* event)
{
    uint32_t endpoint_id = event->transfer_event.endpoint_id;

    if (endpoint_id == 0)
    {
        printk("xhci_device_handle_transfer_event(): incorrect endpoint_id\n");
        return -1;
    }

    if (endpoint_id == 1)
    {
        // TODO; атомарность
        memcpy(&dev->control_completion_trb, event, sizeof(struct xhci_trb));
    } 
    else
    {
        struct usb_endpoint* ep = dev->eps[endpoint_id - 2];
        struct xhci_transfer_ring* ep_ring = (struct xhci_transfer_ring*) ep->transfer_ring;

        size_t cmd_trb_index = (event->cmd_completion.cmd_trb_ptr - ep_ring->trbs_phys) / sizeof(struct xhci_trb);
#ifdef XHCI_LOG_TRANSFER
        printk("XHCI: Transfer completed on slot %i on endpoint %i trb index %i with code %i\n", event->transfer_event.slot_id, endpoint_id, cmd_trb_index, event->transfer_event.completion_code);
#endif
        struct xhci_transfer_ring_completion* compl = &ep_ring->compl[cmd_trb_index];
        memcpy(compl, event, sizeof(struct xhci_trb));

        struct usb_msg* msg = compl->msg;
        if (msg && msg->callback)
        {
            msg->callback(msg);
        }
    }
}

int xhci_device_get_descriptor(struct xhci_device* dev, struct usb_device_descriptor* descr, uint32_t length)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = USB_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = (XHCI_DESCRIPTOR_TYPE_DEVICE << 8);
	req.wIndex = 0;
	req.wLength = length;

    return xhci_device_send_usb_request(dev, &req, descr, length);
}

int xhci_device_get_string_language_descriptor(struct xhci_device* dev, struct usb_string_language_descriptor* descr)
{
    int rc = 0;

    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = USB_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = (XHCI_DESCRIPTOR_TYPE_STRING << 8);
	req.wIndex = 0;
	req.wLength = sizeof(struct usb_descriptor_header);

    // Считываем только заголовок, чтобы узнать реальный размер дескриптора
    rc = xhci_device_send_usb_request(dev, &req, descr, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    req.wLength = descr->header.bLength;

    // Считываем дескриптор
    rc = xhci_device_send_usb_request(dev, &req, descr, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}

int xhci_device_get_string_descriptor(struct xhci_device* dev, uint16_t language_id, uint8_t index, struct usb_string_descriptor* descr)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = USB_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = (XHCI_DESCRIPTOR_TYPE_STRING << 8) | index;
	req.wIndex = language_id;
	req.wLength = sizeof(struct usb_descriptor_header);

    // Считываем только заголовок, чтобы узнать реальный размер дескриптора
    int rc = xhci_device_send_usb_request(dev, &req, descr, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    req.wLength = descr->header.bLength;

    // Считываем дескриптор
    rc = xhci_device_send_usb_request(dev, &req, descr, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}

int xhci_device_get_configuration_descriptor(struct xhci_device* dev, uint8_t configuration, struct usb_configuration_descriptor** descr)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = USB_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = (XHCI_DESCRIPTOR_TYPE_CONFIGURATION << 8) | configuration;
	req.wIndex = 0;
	req.wLength = sizeof(struct usb_descriptor_header);
    
    // Временный объект
    struct usb_configuration_descriptor temp_descriptor;

    // Считываем только заголовок, чтобы узнать реальный размер дескриптора
    int rc = xhci_device_send_usb_request(dev, &req, &temp_descriptor, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    // Считываем дескриптор без данных чтобы узнать его реальный размер
    req.wLength = temp_descriptor.header.bLength;
    rc = xhci_device_send_usb_request(dev, &req, &temp_descriptor, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    // Считываем дескриптор со всеми данными
    *descr = kmalloc(temp_descriptor.wTotalLength);
    req.wLength = temp_descriptor.wTotalLength;
    rc = xhci_device_send_usb_request(dev, &req, *descr, req.wLength);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}

int xhci_device_set_configuration(struct xhci_device* dev, uint8_t configuration)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = USB_DEVICE_REQ_SET_CONFIGURATION;
	req.wValue = configuration;
	req.wIndex = 0;
	req.wLength = 0; // Данный вид запроса не имеет выходных данных

    return xhci_device_send_usb_request(dev, &req, NULL, 0);
}

int xhci_device_get_product_strings(struct xhci_device* xhci_device, struct usb_device* device)
{
    struct usb_device_descriptor* dev_descriptor = &device->descriptor;

    struct usb_string_language_descriptor* lang_descriptor = kmalloc(sizeof(struct usb_string_language_descriptor));
    struct usb_string_descriptor str_descr;

    // Сначала считаем языковые дескрипторы
	int rc = xhci_device_get_string_language_descriptor(xhci_device, lang_descriptor);
	if (rc != 0) 
	{
		printk("XHCI: device string language descriptor request error (%i)!\n", rc);	
        kfree(lang_descriptor);
		return -1;
	}

    uint16_t lang_id = lang_descriptor->lang_ids[0];
    kfree(lang_descriptor);

    // считывание продукта
    memset(&str_descr, 0, sizeof(struct usb_string_descriptor));
    rc = xhci_device_get_string_descriptor(xhci_device, lang_id, dev_descriptor->iProduct, &str_descr);
    if (rc != 0) 
    {
        printk("XHCI: device string product descriptor request error (%i)!\n", rc);	
        return -1;
    }
    device->product = kmalloc(255);
    seize_str(str_descr.unicode_string, device->product);

    // Считывание производителя
    memset(&str_descr, 0, sizeof(struct usb_string_descriptor));
    rc = xhci_device_get_string_descriptor(xhci_device, lang_id, dev_descriptor->iManufacturer, &str_descr);
    if (rc != 0) 
    {
        printk("XHCI: device string manufacturer descriptor request error (%i)!\n", rc);	
        return -1;
    }
    device->manufacturer = kmalloc(255);
    seize_str(str_descr.unicode_string, device->manufacturer);

    // Считывание серийного номера
    memset(&str_descr, 0, sizeof(struct usb_string_descriptor));
    rc = xhci_device_get_string_descriptor(xhci_device, lang_id, dev_descriptor->iSerialNumber, &str_descr);
    if (rc != 0) 
    {
        printk("XHCI: device string serial descriptor request error (%i)!\n", rc);	
        return -1;
    }
    device->serial = kmalloc(255);
    seize_str(str_descr.unicode_string, device->serial);

    return 0;
}

int xhci_device_process_configuration(struct xhci_device* device, uint8_t configuration_idx, struct usb_config** config)
{
    struct usb_configuration_descriptor* config_descriptor = NULL;
    // Считать конфигурацию по номеру
	int rc = xhci_device_get_configuration_descriptor(device, configuration_idx, &config_descriptor);
	if (rc != 0) 
	{
		printk("XHCI: device configuration descriptor (%i) request error (%i)!\n", configuration_idx, rc);	
        kfree(config_descriptor);
		return -1;
	}

	// Включаем конфигурацию
	rc = xhci_device_set_configuration(device, config_descriptor->bConfigurationValue);
	if (rc != 0) 
	{
		printk("XHCI: device configuration descriptor setting (%i) error (%i)!\n", configuration_idx, rc);	
        kfree(config_descriptor);
		return -1;
	}

    // Создаем струтуру конфигурации
    struct usb_config* usb_conf = new_usb_config(config_descriptor);
    *config = usb_conf;

    printk("XHCI: Device configuration %i with %i interfaces\n", configuration_idx, config_descriptor->bNumInterfaces);

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
                printk("XHCI: interface class %i subcl %i prot %i with %i endpoints\n", iface_descr->bInterfaceClass, iface_descr->bInterfaceSubClass, iface_descr->bInterfaceProtocol, iface_descr->bNumEndpoints);

                // Создать общий объект интерфейса
                current_interface = new_usb_interface(iface_descr);

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
                    printk("XHCI: No interface descriptor before this endpoint descriptor");
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
                    printk("XHCI: No endpoint descriptor before this super speed companion descriptor");
                }
                break;
            case USB_DESCRIPTOR_SUPERSPEEDPLUS_ISO_ENDPOINT_COMPANION:
                printk("XHCI: USB_DESCRIPTOR_SUPERSPEEDPLUS_ISO_ENDPOINT_COMPANION\n");
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
				printk("XHCI: unsupported device config %i\n", config_header->bDescriptorType);
		}

		offset += config_header->bLength;
	}

    // Удаляем больше не нужный дескриптор
    kfree(config_descriptor);    
    return 0;
}

int xhci_drv_device_send_usb_request(struct usb_device* dev, struct usb_device_request* req, void* out, uint32_t length)
{
    return xhci_device_send_usb_request(dev->controller_device_data, req, out, length);
}

int xhci_drv_device_configure_endpoint(struct usb_device* dev, struct usb_endpoint* endpoint)
{
    return xhci_device_configure_endpoint(dev->controller_device_data, endpoint);
}

int xhci_drv_device_bulk_msg(struct usb_device* dev, struct usb_endpoint* endpoint, void* data, uint32_t length)
{
    return xhci_device_send_bulk_data(dev->controller_device_data, endpoint, data, length);
}

int xhci_drv_send_async_msg(struct usb_device* dev, struct usb_endpoint* endpoint, struct usb_msg *msg)
{
    return xhci_device_msg_async(dev->controller_device_data, endpoint, msg);
}

int xhci_device_send_usb_request(struct xhci_device* dev, struct usb_device_request* req, void* out, uint32_t length)
{
    struct xhci_trb setup_stage;
    struct xhci_trb data_stage;
    struct xhci_trb status_stage;

    // Для временной памяти
    size_t tmp_data_buffer_pages = 0;
    uintptr_t tmp_data_buffer_phys;
    uintptr_t tmp_data_buffer;

    // Определим значение TRT для Setup Stage
    uint32_t setup_stage_transfer_type = XHCI_SETUP_STAGE_TRT_NO_DATA;
    if (length > 0)
    {
        if (req->transfer_direction == USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST)
        {
            setup_stage_transfer_type = XHCI_SETUP_STAGE_TRT_IN;
        } 
        else 
        {
            setup_stage_transfer_type = XHCI_SETUP_STAGE_TRT_OUT;
        }
    }

    // Table 4.7
    // Определим значение DIR для Data Stage
    uint32_t data_stage_direction = 
        (req->transfer_direction == USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST) ? XHCI_DATA_STAGE_DIRECTION_IN : XHCI_DATA_STAGE_DIRECTION_OUT;

    // Значение DIR для Status Stage
    uint32_t status_stage_direction = 
        (length > 0 && req->transfer_direction == USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST) ? XHCI_STATUS_STAGE_DIRECTION_OUT : XHCI_STATUS_STAGE_DIRECTION_IN;

    // Подготовим структуру Setup Stage
    memset(&setup_stage, 0, sizeof(struct xhci_trb));
    setup_stage.type = XHCI_TRB_TYPE_SETUP;
    setup_stage.setup_stage.trb_transfer_length = 8;
    setup_stage.setup_stage.interrupt_target = 0;
    setup_stage.setup_stage.transfer_type = setup_stage_transfer_type;
    setup_stage.immediate_data = 1;
    setup_stage.setup_stage.interrupt_on_completion = 0;
    memcpy(&setup_stage.setup_stage.req, req, sizeof(struct usb_device_request));

    if (length > 0)
    {
        // Данная операция будет возвращать данные

        // Для начала выделим временную память под выходные данные
        tmp_data_buffer_phys = (uintptr_t) pmm_alloc(length, &tmp_data_buffer_pages);
        tmp_data_buffer = map_io_region(tmp_data_buffer_phys, tmp_data_buffer_pages * PAGE_SIZE);
        memset(tmp_data_buffer, 0, tmp_data_buffer_pages * PAGE_SIZE); 

        // Затем подготовим структуру Data Stage
        memset(&data_stage, 0, sizeof(struct xhci_trb));
        data_stage.type = XHCI_TRB_TYPE_DATA;
        data_stage.data_stage.data_buffer = tmp_data_buffer_phys;
        data_stage.data_stage.trb_transfer_length = length;
        data_stage.data_stage.td_size = 0;
        data_stage.data_stage.interrupt_target = 0;
        data_stage.data_stage.direction = data_stage_direction;
        data_stage.data_stage.chain_bit = 0;
        data_stage.data_stage.interrupt_on_completion = 0;
        data_stage.data_stage.immediate_data = 0;
    }

    // Подготовим структуру Status Stage
    memset(&status_stage, 0, sizeof(struct xhci_trb));
    status_stage.type = XHCI_TRB_TYPE_STATUS;
    status_stage.status_stage.direction = status_stage_direction;
    status_stage.status_stage.chain_bit = 0;
    status_stage.status_stage.interrupt_on_completion = 1;

    // Занулить ответную структуру
    memset(&dev->control_completion_trb, 0, sizeof(struct xhci_trb));

    // Отправить все структуры
    xhci_transfer_ring_enqueue(dev->control_transfer_ring, &setup_stage);
    if (length > 0)
    {
        xhci_transfer_ring_enqueue(dev->control_transfer_ring, &data_stage);
    }
    xhci_transfer_ring_enqueue(dev->control_transfer_ring, &status_stage);

    // Запустить выполнение
    dev->controller->doorbell[dev->slot_id].doorbell = XHCI_DOORBELL_CONTROL_EP_RING;

	while (dev->control_completion_trb.raw.status == 0)
	{
		// wait
	}

    // TODO: костыль, так как нет атомарности
    hpet_sleep(10);

    if (length > 0)
    {
        // Скопировать результат
        memcpy(out, tmp_data_buffer, length);

        // освободить временный буфер
        unmap_io_region(tmp_data_buffer, tmp_data_buffer_pages * PAGE_SIZE);
        pmm_free_pages(tmp_data_buffer_phys, tmp_data_buffer_pages);
    }

    if (dev->control_completion_trb.transfer_event.completion_code != 1)
	{
		return dev->control_completion_trb.transfer_event.completion_code;
	}

    return 0;
}

uint32_t xhci_ilog2(uint32_t val)
{
    uint32_t result;
	for (result = 0; val != 1; result++)
		val = val >> 1;

    return result;
}

uint32_t xhci_ep_compute_interval(struct usb_endpoint* endpoint, uint32_t port_speed)
{
    struct usb_endpoint_descriptor* descr = &endpoint->descriptor;

    uint8_t interval = descr->bInterval;
    uint8_t type = descr->bmAttributes & USB_ENDPOINT_ATTR_TT_MASK;
    // Bulk and Control endpoints never issue NAKs.
    if (type == USB_ENDPOINT_ATTR_TT_CONTROL || type == USB_ENDPOINT_ATTR_TT_BULK)
    {
        return 0;
    }

    // Спизжено из haiku
    switch (port_speed)
    {
        case XHCI_USB_SPEED_FULL_SPEED:
            if (type == USB_ENDPOINT_ATTR_TT_ISOCH)
            {
                // Convert 1-16 into 3-18.
                return MIN(MAX(interval, 1), 16) + 2;
            }
        case XHCI_USB_SPEED_LOW_SPEED:
            // Convert 1ms-255ms into 3-10.
            uint32_t temp = CLAMP(interval * 8, 1, 255);
            uint32_t result = xhci_ilog2(temp);

			return CLAMP(result, 3, 10);
        default:
            // Convert 1-16 into 0-15.
            return MIN(MAX(interval, 1), 16) - 1;
    }

    return 0;
}

uint8_t xhci_ep_get_type(struct usb_endpoint_descriptor* descr)
{
    uint8_t attr = descr->bmAttributes;
    uint8_t type = attr & USB_ENDPOINT_ATTR_TT_MASK; 
    int direction_in = ((descr->bEndpointAddress & USB_ENDPOINT_ADDR_DIRECTION_IN) == USB_ENDPOINT_ADDR_DIRECTION_IN);

    switch (type)
    {
        case USB_ENDPOINT_ATTR_TT_CONTROL:
            return XHCI_ENDPOINT_TYPE_CONTROL;
        case USB_ENDPOINT_ATTR_TT_BULK:
            return direction_in ? XHCI_ENDPOINT_TYPE_BULK_IN : XHCI_ENDPOINT_TYPE_BULK_OUT;
        case USB_ENDPOINT_ATTR_TT_ISOCH:
            return direction_in ? XHCI_ENDPOINT_TYPE_ISOCH_IN : XHCI_ENDPOINT_TYPE_ISOCH_OUT;
        case USB_ENDPOINT_ATTR_TT_INTERRUPT:
            return direction_in ? XHCI_ENDPOINT_TYPE_INT_IN : XHCI_ENDPOINT_TYPE_INT_OUT;
    }

    return XHCI_ENDPOINT_TYPE_NOT_VALID;
}

uint8_t xhci_ep_get_absolute_id(struct usb_endpoint_descriptor* descr)
{
    // Если endpoint в направлении IN - то 1
    int direction_in = ((descr->bEndpointAddress & USB_ENDPOINT_ADDR_DIRECTION_IN) == USB_ENDPOINT_ADDR_DIRECTION_IN);
    // абсолютный номер эндпоинта в таблице контекстов (1 - ...)
    return (descr->bEndpointAddress & 0x0F) * 2 + direction_in;
}

int xhci_device_configure_endpoint(struct xhci_device* dev, struct usb_endpoint* endpoint)
{
    struct usb_endpoint_descriptor* endpoint_descr = &endpoint->descriptor;
    struct xhci_transfer_ring* ring = xhci_create_transfer_ring(XHCI_TRANSFER_RING_ENTITIES);
    xhci_transfer_ring_create_compl_table(ring);
    endpoint->transfer_ring = ring;

    // Сохраним bmAttributes
    uint8_t ep_attr = endpoint_descr->bmAttributes;
    uint8_t ep_type = ep_attr & USB_ENDPOINT_ATTR_TT_MASK; 
    // абсолютный номер эндпоинта в таблице контекстов (1 - ...)
    uint8_t endpoint_id = xhci_ep_get_absolute_id(&endpoint->descriptor);

    // Сохранить указатель в таблице
    dev->eps[endpoint_id - 2] = endpoint;

    // Указатель на input control context
    struct xhci_input_control_context32* input_control_context = dev->input_control_context;

    // Указатель на контекст слота
    struct xhci_slot_context32* slot_ctx = dev->slot_ctx;
    // Указатель на контекст эндпоинта
    struct xhci_endpoint_context32* endpoint_ctx = NULL;

    if (dev->ctx_size == 1)
    {
        // 64 битный контекст
        struct xhci_input_context64* input_ctx = dev->input_ctx;
        endpoint_ctx = &input_ctx->device_ctx.ep[endpoint_id - 2];
    }
    else
    {
        // 32 битный контекст
        struct xhci_input_context32* input_ctx = dev->input_ctx;
        endpoint_ctx = &input_ctx->device_ctx.ep[endpoint_id - 2];
    }

    // Флаги для input control context (изменяются endpoint и slot context)
    input_control_context->add_flags = (1 << endpoint_id) | (1 << 0);
    input_control_context->drop_flags = 0;

    // Сюда надо записать максимальный ID эндпоинта, который настраивался
    dev->max_configured_endpoint_id = MAX(dev->max_configured_endpoint_id, endpoint_id);
    slot_ctx->context_entries = dev->max_configured_endpoint_id;

    // Получить тип эндпоинта
    uint8_t endpoint_type = xhci_ep_get_type(endpoint_descr);
    // для bulk и control - wMaxPacketSize, для остальных - wMaxPacketSize & 0x7FF
    uint32_t max_packet_size = 
        (USB_ENDPOINT_IS_BULK(ep_attr) || USB_ENDPOINT_IS_CONTROL(ep_attr)) ? endpoint_descr->wMaxPacketSize : endpoint_descr->wMaxPacketSize & 0x7FF;

    // По умолчанию - 0
    uint32_t max_burst_size = 0;
    // Для BULK учитываем компаниона
    if (endpoint->ss_companion_present == 1)
    {
        max_burst_size = endpoint->ss_companion.bMaxBurst;
    } 
    else if (ep_type == USB_ENDPOINT_ATTR_TT_ISOCH || ep_type == USB_ENDPOINT_ATTR_TT_INTERRUPT)
    {
        // Для прерываний и изохронных - вот такая формула
        max_burst_size = (max_packet_size & 0x1800) >> 11;
    }

    // XHCI 4.11.7.1 (236)
    uint32_t max_burst_payload = (max_burst_size + 1) * max_packet_size;
    // XHCI 4.14.2 (258)
    uint32_t max_esit_payload = 0;
    if (ep_type == USB_ENDPOINT_ATTR_TT_ISOCH || ep_type == USB_ENDPOINT_ATTR_TT_INTERRUPT)
    {
        if (endpoint->ss_companion_present == TRUE)
        {
            //if (endpoint->ssp_isoc_companion_present && )
            max_esit_payload = endpoint->ss_companion.wBytesPerInterval;
        } 
        else 
        {
            // По умолчанию для USB2 без компанионов
            max_esit_payload = max_packet_size * (max_burst_size + 1);
        }
    }

    // Актуально для interrupt и isoch
    uint32_t interval = xhci_ep_compute_interval(endpoint, dev->port_speed);
    uint32_t avg_trb_len = 0;
    switch (ep_attr & USB_ENDPOINT_ATTR_TT_MASK)
    {
        case USB_ENDPOINT_ATTR_TT_CONTROL:
            avg_trb_len = XHCI_CONTROL_EP_AVG_TRB_LEN;
            break;
        case USB_ENDPOINT_ATTR_TT_ISOCH:
            avg_trb_len = max_packet_size;
            break;
        case USB_ENDPOINT_ATTR_TT_BULK:
        case USB_ENDPOINT_ATTR_TT_INTERRUPT:
            avg_trb_len = max_burst_payload;
    }

    // Для изохронных 0, для остальных 3
    uint32_t error_count = ep_type == USB_ENDPOINT_ATTR_TT_ISOCH ? 0 : 3;

    // Настройка эндпоинта
    size_t ep_ctx_sz = dev->ctx_size == 1 ? sizeof(struct xhci_endpoint_context64) : sizeof(struct xhci_endpoint_context32);
    memset(endpoint_ctx, 0, ep_ctx_sz);
    endpoint_ctx->endpoint_state = XHCI_ENDPOINT_STATE_DISABLED;
    endpoint_ctx->endpoint_type = endpoint_type;
    endpoint_ctx->interval = interval;
    endpoint_ctx->error_count = error_count;
    endpoint_ctx->max_packet_size = max_packet_size;
    endpoint_ctx->max_burst_size = max_burst_size;
    endpoint_ctx->transfer_ring_dequeue_ptr = xhci_transfer_ring_get_cur_phys_ptr(ring);
    endpoint_ctx->dcs = ring->cycle_bit;
    endpoint_ctx->max_esit_payload_lo = max_esit_payload & 0xFFFF;
    endpoint_ctx->max_esit_payload_hi = max_esit_payload >> 16;
    endpoint_ctx->average_trb_length = avg_trb_len;

#ifdef XHCI_LOG_EP_CONFIGURE
    printk("XHCI: Conf ep %i addr %i attr %i type %i max pack %i burst %i esit %i ival %i avg trb %i ring %x\n", 
        endpoint_id, endpoint_descr->bEndpointAddress, ep_attr, endpoint_type, max_packet_size, max_burst_size, max_esit_payload, interval, avg_trb_len, endpoint_ctx->transfer_ring_dequeue_ptr);
#endif

    // команда контроллеру
    return xhci_controller_configure_endpoint(dev->controller, dev->slot_id, dev->input_ctx_phys, FALSE);
}

// Возвращает код возврата USB (0 - успешный) (1024 - stall)
// data - физический адрес
int xhci_device_send_bulk_data(struct xhci_device* dev, struct usb_endpoint* ep, void* data, uint32_t size)
{
    struct xhci_trb transfer_trb;
    transfer_trb.type = XHCI_TRB_TYPE_NORMAL;
    transfer_trb.normal.data_buffer_pointer       = data;
	transfer_trb.normal.trb_transfer_length       = size;
	transfer_trb.normal.td_size                   = 0;
	transfer_trb.normal.interrupt_target          = 0;
	transfer_trb.normal.interrupt_on_completion   = 1;
	transfer_trb.normal.interrupt_on_short_packet = 1;

    struct xhci_transfer_ring* ep_ring = ep->transfer_ring;

    // Добавить в очередь
    size_t index = xhci_transfer_ring_enqueue(ep_ring, &transfer_trb);

    // Сбросить
    struct xhci_transfer_ring_completion* compl = &ep_ring->compl[index];
    memset(compl, 0, sizeof(struct xhci_transfer_ring_completion));

    // Запустить выполнение
    dev->controller->doorbell[dev->slot_id].doorbell = xhci_ep_get_absolute_id(&ep->descriptor);

    // Ожидание
    while (compl->trb.raw.status == 0)
	{
		// wait
	}

    uint32_t status = compl->trb.transfer_event.completion_code; 

    if (status == XHCI_COMPLETION_CODE_STALL)
    {
        return USB_COMPLETION_CODE_STALL;
    }

	if (status != XHCI_COMPLETION_CODE_SUCCESS)
	{
		return status;
	}

    return 0;
}

int xhci_device_msg_async(struct xhci_device* dev, struct usb_endpoint* ep, struct usb_msg* msg)
{
    struct xhci_trb transfer_trb;
    transfer_trb.type = XHCI_TRB_TYPE_NORMAL;
    transfer_trb.normal.data_buffer_pointer       = msg->data;
	transfer_trb.normal.trb_transfer_length       = msg->length;
	transfer_trb.normal.td_size                   = 0;
	transfer_trb.normal.interrupt_target          = 0;
	transfer_trb.normal.interrupt_on_completion   = 1;
    transfer_trb.normal.interrupt_on_short_packet = 1;

    struct xhci_transfer_ring* ep_ring = ep->transfer_ring;

    // Добавить в очередь
    size_t index = xhci_transfer_ring_enqueue(ep_ring, &transfer_trb);

    // Сбросить
    struct xhci_transfer_ring_completion* compl = &ep_ring->compl[index];
    memset(compl, 0, sizeof(struct xhci_transfer_ring_completion));
    compl->msg = msg;

    // Запустить выполнение
    dev->controller->doorbell[dev->slot_id].doorbell = xhci_ep_get_absolute_id(&ep->descriptor);

    return 0;
}