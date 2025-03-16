#ifndef _USB_XHCI_H
#define _USB_XHCI_H

#include "kairax/types.h"

#define XHCI_CMD_RUN        (1 << 0)
#define XHCI_CMD_RESET      (1 << 1)

#define XHCI_STS_HALT       (1 << 0)
#define XHCI_STS_NOT_READY  (1 << 11)

#define XHCI_CR_CYCLE_STATE (1)
#define XHCI_CR_CMD_STOP 	(1 << 1)
#define XHCI_CR_ABORT		(1 << 2)
#define XHCI_RUNNING		(1 << 3)

#define XHCI_EXT_CAP_LEGACY_SUPPORT 1
#define XHCI_EXT_CAP_SUPPORTED_PROTOCOL 2
#define XHCI_EXT_CAP_EXTENDED_PM		3
#define XHCI_EXT_CAP_IO_VIRTALIZATION	4
#define XHCI_EXT_CAP_MESSAGE_INTERRUPT	5


struct xhci_cap_regs {
    //uint32_t caplen_version;
	uint8_t caplen_version;
    uint8_t reserved;
    uint16_t version;
	uint32_t hcsparams1;
	uint32_t hcsparams2;
	uint32_t hcsparams3;
	uint32_t hccparams1;
	uint32_t dboff;
	uint32_t rtsoff;
	uint32_t hccparams2;
} PACKED;

struct xhci_port_regs {
	uint32_t status;
	uint32_t pm_status;
	uint32_t link_info;
	uint32_t lpm_control;
} PACKED;

struct xhci_op_config {
	uint32_t max_device_slots_enabled         	: 8;
	uint32_t u3_enable                  		: 1;
	uint32_t configuration_information_enable 	: 1;
	uint32_t reserved                         	: 22;
} PACKED;

struct xhci_op_regs {
	volatile uint32_t usbcmd;
	volatile uint32_t usbsts;  
	uint32_t pagesize;
	uint32_t pad1[2];
	volatile uint32_t dnctrl; 
	uint32_t crcr_l;
	uint32_t crcr_h;
	uint32_t pad2[4];
	uint32_t dcbaa_l;
	uint32_t dcbaa_h;
	struct xhci_op_config config;  
	uint8_t  padding3[964];
	struct xhci_port_regs portregs[256];
} PACKED;

struct xhci_extended_cap {
	uint32_t capability_id   : 8;
	uint32_t next_capability : 8;
} PACKED;

struct xhci_protocol_cap {
	uint32_t capability_id           : 8;
	uint32_t next_capability         : 8;
	uint32_t minor_revision          : 8;
	uint32_t major_revision          : 8;
	uint32_t name_string             : 32;
	uint32_t compatible_port_offset  : 8;
	uint32_t compatible_port_count   : 8;
	uint32_t protocol_defied         : 12;
	uint32_t protocol_speed_id_count : 4;
	uint32_t protocol_slot_type      : 5;
	uint32_t                         : 27;
} PACKED;

struct xhci_controller 
{
    char* mmio_addr;
    uintptr_t mmio_addr_phys;

    struct xhci_cap_regs*   cap;  
    struct xhci_op_regs*    op;
    struct xhci_port_regs*  port;

	uint8_t slots;
	uint8_t ports;
	uint16_t interrupters;
	uint32_t max_scratchpad_buffers;
	uint32_t pagesize;
	uint16_t ext_cap_offset;

	void* dcbaa;
	void* crcr;
};

// Transfer ring block
struct xhci_trb {
	uint64_t parameter;
	uint32_t status;

	uint16_t cycle    	: 1;
	uint16_t ent      	: 1;
	uint16_t reserved 	: 8;
	uint16_t type 		: 6;

	uint16_t control;
};

void xhci_controller_stop(struct xhci_controller* controller);
void xhci_controller_reset(struct xhci_controller* controller);
int xhci_controller_init_ports(struct xhci_controller* controller);

void xhci_int_hander();

#endif