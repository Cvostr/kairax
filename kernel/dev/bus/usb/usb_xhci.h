#ifndef _USB_XHCI_H
#define _USB_XHCI_H

#include "kairax/types.h"

#define XHCI_CMD_RUN        (1 << 0)
#define XHCI_CMD_RESET      (1 << 1)
#define XHCI_CMD_INTE		(1 << 2) // Interrupts Enable
#define XHCI_CMD_HSEE		(1 << 3) // Host System Error enable

#define XHCI_STS_HALT       	(1 << 0)
#define XHCI_STS_HSE 			(1 << 2) // Host System Error
#define XHCI_STS_EINTERRUPT 	(1 << 3)
#define XHCI_STS_PORT_CHANGE	(1 << 4)
#define XHCI_STS_SSS			(1 << 8) // Save State Status
#define XHCI_STS_RSS			(1 << 9) // Restore State Status
#define XHCI_STS_SRE			(1 << 10) // Save Restore Error
#define XHCI_STS_NOT_READY  	(1 << 11)
#define XHCI_STS_HCE			(1 << 12) // Host Controller Error

#define XHCI_PORTSC_CCS			(1) 		// Current Connect Status
#define XHCI_POSRTSC_PED		(1 << 1) 	// Port Enabled-Disabled
#define XHCI_PORTSC_OCA			(1 << 3) 	// Overcurrent Active
#define XHCI_PORTSC_PORTRESET	(1 << 4)
#define XHCI_PORTSC_PORTPOWER	(1 << 9)
#define XHCI_PORTSC_CSC			(1 << 17)	// Connect Status Change
#define XHCI_PORTSC_PEC			(1 << 18)	// Port Enable Disable Change
#define XHCI_PORTSC_PRC			(1 << 21)	// Port Reset Change
#define XHCI_PORTSC_WPR			(1 << 31)	// Port Warm Reset on USB3

#define XHCI_CR_CYCLE_STATE (1)
#define XHCI_CR_CMD_STOP 	(1 << 1)
#define XHCI_CR_ABORT		(1 << 2)
#define XHCI_RUNNING		(1 << 3)

#define XHCI_IMAN_INTERRUPT_PENDING (1 << 0)
#define XHCI_IMAN_INTERRUPT_ENABLE 	(1 << 1)

#define XHCI_EXT_CAP_LEGACY_SUPPORT 1
#define XHCI_EXT_CAP_SUPPORTED_PROTOCOL 2
#define XHCI_EXT_CAP_EXTENDED_PM		3
#define XHCI_EXT_CAP_IO_VIRTALIZATION	4
#define XHCI_EXT_CAP_MESSAGE_INTERRUPT	5

#define XHCI_EVENT_HANDLER_BUSY		(1 << 3)

// TRB types
#define XHCI_TRB_TYPE_NORMAL		1
#define XHCI_TRB_TYPE_SETUP			2
#define XHCI_TRB_TYPE_DATA			3
#define XHCI_TRB_TYPE_STATUS		4
#define XHCI_TRB_TYPE_LINK			6
// TRB command types
#define XHCI_TRB_ENABLE_SLOT_CMD		9
#define XHCI_TRB_DISABLE_SLOT_CMD		10
#define XHCI_TRB_ADDRESS_DEVICE_CMD 	11
#define XHCI_TRB_CONFIGURE_ENDPOINT_CMD	12
#define XHCI_TRB_GET_PORT_BANDWIDTH_CMD	21
#define XHCI_TRB_NO_OP_CMD				23

struct xhci_cap_regs {
	uint8_t caplen;
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
	volatile uint64_t crcr;
	uint32_t pad2[4];
	volatile uint64_t dcbaa;
	struct xhci_op_config config;  
	uint8_t  padding3[964];
	struct xhci_port_regs portregs[256];
} PACKED;

// XHCI Extended Capabilities
struct xhci_extended_cap {
	uint32_t capability_id   : 8;
	uint32_t next_capability : 8;
} PACKED;

struct xhci_legacy_support_cap
{
	uint32_t capability_id          : 8;
	uint32_t next_capability        : 8;
	uint32_t bios_owned_semaphore 	: 1;
	uint32_t                        : 7;
	uint32_t os_owned_semaphore   	: 1;
	uint32_t                        : 7;
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
// --------------------------

struct xhci_interrupter
{
	uint32_t iman;
	uint32_t imod;
	uint32_t erstsz;
	uint32_t reserved;
	volatile uint64_t erstba;
	volatile uint64_t erdp;
} PACKED;

struct xhci_runtime_regs {

	uint32_t mfindex;
	uint32_t reserved[7];
	struct xhci_interrupter interrupters[];
} PACKED;

// Transfer ring block
struct xhci_trb {

	uint64_t parameter;
	uint32_t status;

	union 
	{
		struct {
			uint16_t cycle    	: 1;
			uint16_t ent      	: 1;	// Evaluate next TRB
			// Bits
			uint32_t interrupt_on_short_packet : 1;
			uint32_t no_snoop                  : 1;
			uint32_t chain_bit                 : 1;
			uint32_t interrupt_on_completion   : 1;
			uint32_t immediate_data            : 1;
			uint32_t reserved                  : 2;
			uint32_t block_event_interrupt     : 1;
			// TRB type
			uint16_t type 		: 6;
		
			uint16_t control;
		};

		struct 
		{
			uint32_t control;
		} raw;

		struct 
		{
			uint16_t cycle    		: 1;
			uint16_t cycle_enable 	: 1;
			uint16_t reserved 		: 8;
			uint16_t type 			: 6;
			uint16_t reserved2;
		} link;
	};
	
} PACKED;

struct xhci_event_ring_seg_table_entry {
	uint64_t rsba;
	uint32_t rsz;
	uint32_t rsvd;
} PACKED;

union xhci_doorbell_register {

	uint32_t doorbell;
	struct {
		uint32_t target : 8; // 0 for host command ring
		uint32_t rsvdZ : 8;
		uint32_t streamID : 16; // Should be 0 for host controller doorbells
	} PACKED;
} PACKED;

struct xhci_command_ring
{
	size_t trbs_num;
	size_t next_trb_index;
	
	struct xhci_trb* 	trbs;
	uintptr_t 			trbs_phys;

	uint8_t	cycle_bit;
};

struct xhci_command_ring *xhci_create_command_ring(size_t ntrbs);
void xhci_command_enqueue(struct xhci_command_ring *ring, struct xhci_trb* trb);

struct xhci_event_ring
{
	size_t trb_count;
	size_t segment_count;

	size_t next_trb_index;
	uint8_t cycle_bit;

	struct xhci_trb* 	trbs;
	uintptr_t 			trbs_phys;

	struct xhci_event_ring_seg_table_entry* segment_table;
	uintptr_t segment_table_phys;
};

struct xhci_event_ring *xhci_create_event_ring(size_t ntrbs, size_t segments);
int xhci_event_ring_deque(struct xhci_event_ring *ring, struct xhci_trb *trb);

struct xhci_port_desc
{
	/*uint8_t revision_major;
	uint8_t revision_minor;
	uint8_t slot_id;*/
	struct xhci_protocol_cap* proto_cap;
	struct xhci_port_regs*  port_regs;
};

struct xhci_controller 
{
    char* mmio_addr;
    uintptr_t mmio_addr_phys;

    struct xhci_cap_regs*   cap;  
    struct xhci_op_regs*    op;
    struct xhci_port_regs*  ports_regs;
	struct xhci_runtime_regs* runtime;
	union xhci_doorbell_register* doorbell;

	//uintptr_t ersts_phys;
	//struct xhci_event_ring_seg_table_entry* ersts;

	uint8_t slots;
	uint8_t ports_num;
	uint16_t interrupters;
	uint32_t max_scratchpad_buffers;
	uint32_t pagesize;
	uint16_t ext_cap_offset;

	struct xhci_command_ring *cmdring;
	struct xhci_event_ring* event_ring;
	struct xhci_port_desc *ports;

	uintptr_t* dcbaa;
	//void* event_ring;
};

void xhci_controller_stop(struct xhci_controller* controller);
int xhci_controller_reset(struct xhci_controller* controller);
int xhci_controller_start(struct xhci_controller* controller);
int xhci_controller_check_ext_caps(struct xhci_controller* controller);
int xhci_controller_init_scratchpad(struct xhci_controller* controller);
int xhci_controller_init_interrupts(struct xhci_controller* controller, struct xhci_event_ring* event_ring);

void xhci_controller_init_ports(struct xhci_controller* controller);

void xhci_int_hander();

#endif