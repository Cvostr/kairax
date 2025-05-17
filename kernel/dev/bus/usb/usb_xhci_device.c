#include "usb_xhci.h"
#include "usb_xhci_contexts.h"
#include "mem/kheap.h"
#include "string.h"
#include "mem/iomem.h"
#include "mem/pmm.h"

#define XHCI_TRANSFER_RING_ENTIRIES 256

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
        struct xhci_input_context64* input_ctx = dev->input_ctx;
        dev->input_control_context = &input_ctx->input_ctrl_ctx;
        dev->slot_ctx = &input_ctx->device_ctx.slot_context;
        dev->control_endpoint_ctx = &input_ctx->device_ctx.control_ep_context;
    }

    dev->transfer_ring = xhci_create_transfer_ring(XHCI_TRANSFER_RING_ENTIRIES);

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
    slot_ctx->root_hub_port_num = dev->port_id;
    slot_ctx->route_string = 0;
    slot_ctx->interrupter_target = 0;

    struct xhci_endpoint_context32* ctrl_endpoint_ctx = dev->control_endpoint_ctx;
    ctrl_endpoint_ctx->endpoint_state = XHCI_ENDPOINT_STATE_DISABLED;
    ctrl_endpoint_ctx->endpoint_type = XHCI_ENDPOINT_TYPE_CONTROL;
    ctrl_endpoint_ctx->interval = 0;
    ctrl_endpoint_ctx->error_count = 3;
    ctrl_endpoint_ctx->max_packet_size = max_packet_size;
    ctrl_endpoint_ctx->transfer_ring_dequeue_ptr = xhci_transfer_ring_get_cur_phys_ptr(dev->transfer_ring);
    ctrl_endpoint_ctx->dcs = dev->transfer_ring->cycle_bit;
    ctrl_endpoint_ctx->max_esit_payload_lo = 0;
    ctrl_endpoint_ctx->max_esit_payload_hi = 0;
    ctrl_endpoint_ctx->average_trb_length = 8;
}

void xhci_device_send_usb_request(struct xhci_device* dev, struct xhci_device_request* req, void* out, uint32_t length)
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
    setup_stage.setup_stage.req = *req;
    setup_stage.setup_stage.trb_transfer_length = 8;
    setup_stage.setup_stage.interrupt_target = 0;
    setup_stage.setup_stage.transfer_type = setup_stage_transfer_type;
    setup_stage.immediate_data = 1;
    setup_stage.setup_stage.interrupt_on_completion = 0;

    size_t tmp_data_buffer_pages = 0;
    uintptr_t tmp_data_buffer_phys = (uintptr_t) pmm_alloc(length, &tmp_data_buffer_pages);
	uintptr_t tmp_data_buffer = map_io_region(dev->input_ctx_phys, tmp_data_buffer_pages * PAGE_SIZE);
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
    data_stage.data_stage.chain_bit = 1;
    data_stage.data_stage.interrupt_on_completion = 0;
    data_stage.data_stage.immediate_data = 0;

/*
    struct xhci_trb event_data_first;
    memset(&data_stage, 0, sizeof(struct xhci_trb));
    event_data_first.type = XHCI_TRB_TYPE_EVENT_DATA;
    event_data_first.data = xhci_get_physical_addr(transfer_status_buffer);
    event_data_first.interrupter_target = 0;
    event_data_first.chain = 0;
    event_data_first.ioc = 1;
*/

    struct xhci_trb status_stage;
    memset(&status_stage, 0, sizeof(struct xhci_trb));
    status_stage.status_stage.direction = status_stage_direction;
    status_stage.status_stage.chain_bit = 0;
    status_stage.status_stage.interrupt_on_completion = 1;

    xhci_transfer_ring_enqueue(dev->transfer_ring, &setup_stage);
    xhci_transfer_ring_enqueue(dev->transfer_ring, &data_stage);
    xhci_transfer_ring_enqueue(dev->transfer_ring, &status_stage);

    printk("XHC: QUEUED\n");
    hpet_sleep(300);

    dev->controller->doorbell[dev->slot_id].doorbell = XHCI_DOORBELL_CONTROL_EP_RING;

    // Скопировать результат
    memcpy(out, tmp_data_buffer, length);

    // освободить временный буфер
    unmap_io_region(tmp_data_buffer, tmp_data_buffer_pages * PAGE_SIZE);
    pmm_free_pages(tmp_data_buffer_phys, tmp_data_buffer_pages * PAGE_SIZE);
}