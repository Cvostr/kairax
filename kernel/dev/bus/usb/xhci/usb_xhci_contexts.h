#ifndef _USB_XHCI_CONTEXTS_H
#define _USB_XHCI_CONTEXTS_H

#include "kairax/types.h"

struct xhci_slot_context32 {
    union {
        struct {
            uint32_t route_string   : 20;
            uint32_t speed          : 4;
            // Reserved
            uint32_t rz             : 1;
            uint32_t mtt            : 1;
            // Hub. This flag is set to '1' by software if this device is a USB hub, or '0' if it is a USB function.
            uint32_t hub            : 1;
            uint32_t context_entries : 5;
        };
        uint32_t dword0;
    };

    union {
        struct {
            uint16_t    max_exit_latency;
            uint8_t     root_hub_port_num;
            uint8_t     port_count;
        };
        uint32_t dword1;
    };

    union {
        struct {
            uint32_t parent_hub_slot_id  : 8;
            uint32_t parent_port_number  : 8;
            uint32_t tt_think_time       : 2;
            // Reserved
            uint32_t rsvd0               : 4;
            uint32_t interrupter_target  : 10;
        };
        uint32_t dword2;
    };

    union {
        struct {
            uint32_t device_address  : 8;
            // Reserved
            uint32_t rsvd1           : 19;
            uint32_t slot_state      : 5;
        };
        uint32_t dword3;
    };
    uint32_t rsvdz[4];
} PACKED;

struct xhci_slot_context64 {
    union {
        struct {
            uint32_t route_string   : 20;
            uint32_t speed          : 4;
            // Reserved
            uint32_t rz             : 1;
            uint32_t mtt            : 1;
            // Hub. This flag is set to '1' by software if this device is a USB hub, or '0' if it is a USB function.
            uint32_t hub            : 1;
            uint32_t context_entries : 5;
        };
        uint32_t dword0;
    };

    union {
        struct {
            uint16_t    max_exit_latency;
            uint8_t     root_hub_port_num;
            uint8_t     port_count;
        };
        uint32_t dword1;
    };

    union {
        struct {
            uint32_t parent_hub_slot_id  : 8;
            uint32_t parent_port_number  : 8;
            uint32_t tt_think_time       : 2;
            // Reserved
            uint32_t rsvd0               : 4;
            uint32_t interrupter_target  : 10;
        };
        uint32_t dword2;
    };

    union {
        struct {
            uint32_t device_address  : 8;
            // Reserved
            uint32_t rsvd1           : 19;
            uint32_t slot_state      : 5;
        };
        uint32_t dword3;
    };
    uint32_t rsvdz[4];
    // Должна быть выравнена до 64 байт, если указан HCC1.CSZ
    uint32_t padding[8];
} PACKED;

// Section 4.8.3
#define XHCI_ENDPOINT_STATE_DISABLED    0
#define XHCI_ENDPOINT_STATE_RUNNING     1
#define XHCI_ENDPOINT_STATE_HALTED      2
#define XHCI_ENDPOINT_STATE_STOPPED     3
#define XHCI_ENDPOINT_STATE_ERROR       4

#define XHCI_ENDPOINT_TYPE_NOT_VALID    0
#define XHCI_ENDPOINT_TYPE_ISOCH_OUT    1
#define XHCI_ENDPOINT_TYPE_BULK_OUT     2
#define XHCI_ENDPOINT_TYPE_INT_OUT      3
#define XHCI_ENDPOINT_TYPE_CONTROL      4
#define XHCI_ENDPOINT_TYPE_ISOCH_IN     5
#define XHCI_ENDPOINT_TYPE_BULK_IN      6
#define XHCI_ENDPOINT_TYPE_INT_IN       7
struct xhci_endpoint_context32 {
    union {
        struct {
            uint32_t endpoint_state        : 3;
            uint32_t rsvd0                 : 5;
            uint32_t mult                  : 2;
            uint32_t max_primary_streams   : 5;
            uint32_t linear_stream_array   : 1;
            uint32_t interval              : 8;
            uint32_t max_esit_payload_hi   : 8;
        };
        uint32_t dword0;
    };

    union {
        struct {
            // Reserved
            uint32_t rsvd1                   : 1;
            uint32_t error_count             : 2;
            uint32_t endpoint_type           : 3;
            uint32_t rsvd2                   : 1;
            uint32_t host_initiate_disable   : 1;
            uint32_t max_burst_size          : 8;
            uint32_t max_packet_size         : 16;
        };
        uint32_t dword1;
    };

    union {
        struct {
            uint64_t dcs                            : 1;
            uint64_t rsvd3                          : 3;
            uint64_t tr_dequeue_ptr_address_bits    : 60;
        };
        uint64_t transfer_ring_dequeue_ptr;        
    };

    union {
        struct {
            uint16_t average_trb_length;
            uint16_t max_esit_payload_lo;
        };
        uint32_t dword4;
    };

    uint32_t padding1[3];
} PACKED;

struct xhci_endpoint_context64 {
    union {
        struct {
            uint32_t endpoint_state        : 3;
            uint32_t rsvd0                 : 5;
            uint32_t mult                  : 2;
            uint32_t max_primary_streams   : 5;
            uint32_t linear_stream_array   : 1;
            uint32_t interval              : 8;
            uint32_t max_esit_payload_hi   : 8;
        };
        uint32_t dword0;
    };

    union {
        struct {
            // Reserved
            uint32_t rsvd1                   : 1;
            uint32_t error_count             : 2;
            uint32_t endpoint_type           : 3;
            uint32_t rsvd2                   : 1;
            uint32_t host_initiate_disable   : 1;
            uint32_t max_burst_size          : 8;
            uint32_t max_packet_size         : 16;
        };
        uint32_t dword1;
    };

    union {
        struct {
            uint64_t dcs                      : 1;
            uint64_t rsvd3                    : 3;
            uint64_t tr_dequeue_ptr_address_bits    : 60;
        };
        uint64_t transfer_ring_dequeue_ptr;        
    };

    union {
        struct {
            uint16_t average_trb_length;
            uint16_t max_esit_payload_lo;
        };
        uint32_t dword4;
    };

    uint32_t padding1[3];

    // Должна быть выравнена до 64 байт, если указан HCC1.CSZ
    uint32_t padding[8];
} PACKED;


struct xhci_input_control_context32 {

    uint32_t drop_flags;
    uint32_t add_flags;

    uint32_t rsvd[5];
    uint8_t config_value;

    uint8_t interface_number;
    uint8_t alternate_setting;

    // Reserved and zero'd
    uint8_t rsvdZ;
} PACKED;

struct xhci_input_control_context64 {

    uint32_t drop_flags;
    uint32_t add_flags;

    uint32_t rsvd[5];
    uint8_t config_value;

    uint8_t interface_number;
    uint8_t alternate_setting;

    // Reserved and zero'd
    uint8_t rsvdZ;

    // Должна быть выравнена до 64 байт, если указан HCC1.CSZ
    uint32_t padding[8];
} PACKED;

struct xhci_device_context32 {

    // Slot context
    struct xhci_slot_context32 slot_context;

    // Primary control endpoint
    struct xhci_endpoint_context32 control_ep_context;

    // Optional communication endpoints
    struct xhci_endpoint_context32 ep[30];
} PACKED;

struct xhci_device_context64 {

    // Slot context
    struct xhci_slot_context64 slot_context;

    // Primary control endpoint
    struct xhci_endpoint_context64 control_ep_context;

    // Optional communication endpoints
    struct xhci_endpoint_context64 ep[30];
} PACKED;

struct xhci_input_context32 {
    
    struct xhci_input_control_context32 input_ctrl_ctx;

    struct xhci_device_context32    device_ctx;
} PACKED;

struct xhci_input_context64 {
    
    struct xhci_input_control_context64 input_ctrl_ctx;

    struct xhci_device_context64    device_ctx;
} PACKED;

#endif