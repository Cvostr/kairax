#include "usb_xhci.h"
#include "usb_xhci_contexts.h"
#include "mem/kheap.h"
#include "string.h"
#include "mem/iomem.h"
#include "mem/pmm.h"

#define XHCI_TRANSFER_RING_ENTITIES 256

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

    // TODO: use refcount
    kfree(dev);
}

int xhci_device_init_contexts(struct xhci_device* dev)
{
    size_t input_context_sz = dev->ctx_size == 1 ? sizeof(struct xhci_input_context64) : sizeof(struct xhci_input_context32);   
    size_t device_context_sz = dev->ctx_size == 1 ? sizeof(struct xhci_device_context64) : sizeof(struct xhci_device_context32);   

    dev->input_ctx_phys = (uintptr_t) pmm_alloc(input_context_sz, NULL);
	dev->input_ctx = map_io_region(dev->input_ctx_phys, input_context_sz);
    memset(dev->input_ctx, 0, input_context_sz);

    dev->device_ctx_phys = (uintptr_t) pmm_alloc(device_context_sz, NULL);
	dev->device_ctx = map_io_region(dev->device_ctx_phys, device_context_sz);
    memset(dev->device_ctx, 0, device_context_sz);

    if (dev->ctx_size == 1)
    {
        struct xhci_input_context64* input_ctx = dev->input_ctx;
        dev->input_control_context = &input_ctx->input_ctrl_ctx;
        dev->slot_ctx = &input_ctx->device_ctx.slot_context;
        dev->control_endpoint_ctx = &input_ctx->device_ctx.control_ep_context;
    }
    else
    {
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
    input_control_context->add_flags = (1 << 0) | (1 << 1);
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
    ctrl_endpoint_ctx->average_trb_length = 8;
}

int xhci_device_handle_transfer_event(struct xhci_device* dev, struct xhci_trb* event)
{
    uint32_t endpoint_id = event->transfer_event.endpoint_id;

    if (endpoint_id == 0)
    {
        printk("XHCI: incorrect endpoint_id\n");
        return -1;
    }

    if (endpoint_id == 1)
    {
        // TODO; атомарность
        memcpy(&dev->control_completion_trb, event, sizeof(struct xhci_trb));
    }
}

int xhci_device_get_descriptor(struct xhci_device* dev, struct usb_device_descriptor* descr, uint32_t length)
{
    struct xhci_device_request req;
	req.type = XHCI_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = XHCI_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = XHCI_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = XHCI_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = (XHCI_DESCRIPTOR_TYPE_DEVICE << 8);
	req.wIndex = 0;
	req.wLength = length;

    return xhci_device_send_usb_request(dev, &req, descr, length);
}

int xhci_device_get_string_language_descriptor(struct xhci_device* dev, struct usb_string_language_descriptor* descr)
{
    int rc = 0;

    struct xhci_device_request req;
	req.type = XHCI_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = XHCI_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = XHCI_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = XHCI_DEVICE_REQ_GET_DESCRIPTOR;
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
    int rc = 0;

    struct xhci_device_request req;
	req.type = XHCI_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = XHCI_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = XHCI_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = XHCI_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = (XHCI_DESCRIPTOR_TYPE_STRING << 8) | index;
	req.wIndex = language_id;
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

int xhci_device_get_configuration_descriptor(struct xhci_device* dev, struct usb_configuration_descriptor* descr)
{
    
}

int xhci_device_send_usb_request(struct xhci_device* dev, struct xhci_device_request* req, void* out, uint32_t length)
{
    // Определим значение TRT для Setup Stage
    uint32_t setup_stage_transfer_type = XHCI_SETUP_STAGE_TRT_NO_DATA;
    if (length > 0)
    {
        if (req->transfer_direction == XHCI_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST)
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
        (req->transfer_direction == XHCI_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST) ? XHCI_DATA_STAGE_DIRECTION_IN : XHCI_DATA_STAGE_DIRECTION_OUT;

    // Значение DIR для Status Stage
    uint32_t status_stage_direction = 
        (length > 0 && req->transfer_direction == XHCI_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST) ? XHCI_STATUS_STAGE_DIRECTION_OUT : XHCI_STATUS_STAGE_DIRECTION_IN;

    // Подготовим структуру Setup Stage
    struct xhci_trb setup_stage;
    memset(&setup_stage, 0, sizeof(struct xhci_trb));
    setup_stage.type = XHCI_TRB_TYPE_SETUP;
    setup_stage.setup_stage.trb_transfer_length = 8;
    setup_stage.setup_stage.interrupt_target = 0;
    setup_stage.setup_stage.transfer_type = setup_stage_transfer_type;
    setup_stage.immediate_data = 1;
    setup_stage.setup_stage.interrupt_on_completion = 0;
    memcpy(&setup_stage.setup_stage.req, req, sizeof(struct xhci_device_request));

    size_t tmp_data_buffer_pages = 0;
    uintptr_t tmp_data_buffer_phys = (uintptr_t) pmm_alloc(length, &tmp_data_buffer_pages);
	uintptr_t tmp_data_buffer = map_io_region(tmp_data_buffer_phys, tmp_data_buffer_pages * PAGE_SIZE);
    memset(tmp_data_buffer, 0, tmp_data_buffer_pages * PAGE_SIZE); 

    // Подготовим структуру Data Stage
    struct xhci_trb data_stage;
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

    // Подготовим структуру Status Stage
    struct xhci_trb status_stage;
    memset(&status_stage, 0, sizeof(struct xhci_trb));
    status_stage.type = XHCI_TRB_TYPE_STATUS;
    status_stage.status_stage.direction = status_stage_direction;
    status_stage.status_stage.chain_bit = 0;
    status_stage.status_stage.interrupt_on_completion = 1;

    // Занулить ответную структуру
    memset(&dev->control_completion_trb, 0, sizeof(struct xhci_trb));

    // Отправить все структуры
    xhci_transfer_ring_enqueue(dev->control_transfer_ring, &setup_stage);
    xhci_transfer_ring_enqueue(dev->control_transfer_ring, &data_stage);
    xhci_transfer_ring_enqueue(dev->control_transfer_ring, &status_stage);

    // Запустить выполнение
    dev->controller->doorbell[dev->slot_id].doorbell = XHCI_DOORBELL_CONTROL_EP_RING;

	while (dev->control_completion_trb.raw.status == 0)
	{
		// wait
	}

    // TODO: костыль, так как нет атомарности
    hpet_sleep(10);

    // Скопировать результат
    memcpy(out, tmp_data_buffer, length);

    // освободить временный буфер
    unmap_io_region(tmp_data_buffer, tmp_data_buffer_pages * PAGE_SIZE);
    pmm_free_pages(tmp_data_buffer_phys, tmp_data_buffer_pages * PAGE_SIZE);

    if (dev->control_completion_trb.transfer_event.completion_code != 1)
	{
		return dev->control_completion_trb.transfer_event.completion_code;
	}

    return 0;
}