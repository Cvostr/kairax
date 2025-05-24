#ifndef _USB_XHCI_H
#define _USB_XHCI_H

#include "kairax/types.h"
#include "../usb_descriptors.h"

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

// Port Status
#define XHCI_PORTSC_CCS			(1) 		// Current Connect Status
#define XHCI_POSRTSC_PED		(1 << 1) 	// Port Enabled-Disabled
#define XHCI_PORTSC_OCA			(1 << 3) 	// Overcurrent Active
#define XHCI_PORTSC_PORTRESET	(1 << 4)
#define XHCI_PORTSC_PORTPOWER	(1 << 9)
#define XHCI_PORTSC_CSC			(1 << 17)	// Connect Status Change
#define XHCI_PORTSC_PEC			(1 << 18)	// Port Enable Disable Change
#define XHCI_PORTSC_WRC			(1 << 19)	// Warm Port Reset Change
#define XHCI_PORTSC_PRC			(1 << 21)	// Port Reset Change
#define XHCI_PORTSC_WPR			(1U << 31)	// Port Warm Reset on USB3

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
#define XHCI_TRB_TYPE_EVENT_DATA	7
// TRB command types
#define XHCI_TRB_ENABLE_SLOT_CMD		9
#define XHCI_TRB_DISABLE_SLOT_CMD		10
#define XHCI_TRB_ADDRESS_DEVICE_CMD 	11
#define XHCI_TRB_CONFIGURE_ENDPOINT_CMD	12
#define XHCI_TRB_GET_PORT_BANDWIDTH_CMD	21
#define XHCI_TRB_NO_OP_CMD				23
// TRB events
#define XHCI_TRB_TRANSFER_EVENT				32
#define XHCI_TRB_CMD_COMPLETION_EVENT		33
#define XHCI_TRB_PORT_STATUS_CHANGE_EVENT	34
#define XHCI_TRB_BANDWIDTH_REQUEST_EVENT	35
#define XHCI_TRB_DOORBELL_EVENT				36
#define XHCI_TRB_HOST_CONTROLLER_EVENT		37
#define XHCI_TRB_DEVICE_NOTIFICATION_EVENT	38
// TODO: wrap event (39)

// XHCI - SMI
#define XHCI_LEGACY_SMI_ENABLE               (1 << 0)   // USB SMI Enable
#define XHCI_LEGACY_SMI_ON_OS_OWNERSHIP      (1 << 13)  // SMI on OS Ownership Enable
#define XHCI_LEGACY_SMI_ON_HOST_ERROR        (1 << 4)   // SMI on Host System Error
#define XHCI_LEGACY_SMI_ON_PCI_COMMAND       (1 << 14)  // SMI on PCI Command
#define XHCI_LEGACY_SMI_ON_BAR               (1 << 15)  // SMI on BAR (Base Address Register)

#define XHCI_DOORBELL_COMMAND_RING       0
#define XHCI_DOORBELL_CONTROL_EP_RING    1

// USB Speeds
#define XHCI_USB_SPEED_UNDEFINED            0
#define XHCI_USB_SPEED_FULL_SPEED           1 // 12 MB/s USB 2.0
#define XHCI_USB_SPEED_LOW_SPEED            2 // 1.5 Mb/s USB 2.0
#define XHCI_USB_SPEED_HIGH_SPEED           3 // 480 Mb/s USB 2.0
#define XHCI_USB_SPEED_SUPER_SPEED          4 // 5 Gb/s (Gen1 x1) USB 3.0
#define XHCI_USB_SPEED_SUPER_SPEED_PLUS     5 // 10 Gb/s (Gen2 x1) USB 3.1

#define XHCI_DESCRIPTOR_TYPE_DEVICE			1
#define XHCI_DESCRIPTOR_TYPE_CONFIGURATION	2
#define XHCI_DESCRIPTOR_TYPE_STRING			3
#define XHCI_DESCRIPTOR_TYPE_INTERFACE		4
#define XHCI_DESCRIPTOR_TYPE_ENDPOINT		5
#define XHCI_DESCRIPTOR_TYPE_DEVICE_QUALIFIER	6
#define XHCI_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIG	7
#define XHCI_DESCRIPTOR_TYPE_INTERFACE_POWER	8			

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
	volatile uint32_t status;
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
	// USBLEGSUP
	uint32_t capability_id          : 8;
	uint32_t next_capability        : 8;
	uint32_t bios_owned_semaphore 	: 1;
	uint32_t rsvdP                  : 7;
	uint32_t os_owned_semaphore   	: 1;
	uint32_t rsvdP1                 : 7;
	// USBLEGCTLSTS
	uint32_t usblegctlsts;
	
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

#define XHCI_DEVICE_REQ_RECIPIENT_DEVICE	0
#define XHCI_DEVICE_REQ_RECIPIENT_INTERFACE	1
#define XHCI_DEVICE_REQ_RECIPIENT_ENDPOINT	2
#define XHCI_DEVICE_REQ_RECIPIENT_OTHER		3
#define XHCI_DEVICE_REQ_RECIPIENT_RESERVED	4

#define XHCI_DEVICE_REQ_TYPE_STANDART		0
#define XHCI_DEVICE_REQ_TYPE_CLASS			1
#define XHCI_DEVICE_REQ_TYPE_VENDOR			2
#define XHCI_DEVICE_REQ_TYPE_RSVD			3

#define XHCI_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE	0
#define XHCI_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST	1

#define XHCI_DEVICE_REQ_GET_STATUS			0
#define XHCI_DEVICE_REQ_CLEAR_FEATURE		1
#define XHCI_DEVICE_REQ_SET_FEATURE			2
#define XHCI_DEVICE_REQ_SET_ADDRESS			5
#define XHCI_DEVICE_REQ_GET_DESCRIPTOR		6
#define XHCI_DEVICE_REQ_SET_DESCRIPTOR		7
#define XHCI_DEVICE_REQ_GET_CONFIGURATION	8
#define XHCI_DEVICE_REQ_SET_CONFIGURATION	9
#define XHCI_DEVICE_REQ_GET_INTERFACE		10
#define XHCI_DEVICE_REQ_SET_INTERFACE		11
#define XHCI_DEVICE_REQ_SYNC_FRAME			12
struct xhci_device_request {
    
    uint8_t recipient           : 5;
    uint8_t type                : 2;
	uint8_t transfer_direction  : 1;

    uint8_t 	bRequest;
    uint16_t 	wValue;
    uint16_t 	wIndex;
    uint16_t 	wLength;
};

#define XHCI_SETUP_STAGE_TRT_NO_DATA	0
#define XHCI_SETUP_STAGE_TRT_RESERVED	1
#define XHCI_SETUP_STAGE_TRT_OUT		2
#define XHCI_SETUP_STAGE_TRT_IN			3

#define XHCI_DATA_STAGE_DIRECTION_OUT	0
#define XHCI_DATA_STAGE_DIRECTION_IN	1

#define XHCI_STATUS_STAGE_DIRECTION_OUT	0
#define XHCI_STATUS_STAGE_DIRECTION_IN	1

// Transfer ring block
struct xhci_trb {

	union 
	{
		struct {
			uint64_t parameter;
			uint32_t status;

			uint16_t cycle_bit  : 1;
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
			uint64_t parameter;
			uint32_t status;
			uint32_t control;
		} raw;

		struct 
		{
			uint64_t address;
			uint32_t status;
			uint16_t cycle_bit    	: 1;
			uint16_t cycle_enable 	: 1;
			uint16_t reserved 		: 8;
			uint16_t type 			: 6;
			uint16_t reserved2;
		} link;

		struct
		{
			uint32_t rsvd0   : 24;
			uint32_t port_id : 8;

			uint32_t rsvd1;

			uint32_t rsvd2           : 24;
        	uint32_t completion_code : 8;

			uint32_t cycle_bit  : 1;
        	uint32_t rsvd3      : 9;
        	uint32_t trb_type   : 6;
        	uint32_t rsvd4      : 16;
		} port_status_change;

		struct 
		{
			uint64_t input_context_pointer     : 64;
			uint32_t                           : 32;
			uint32_t cycle_bit                 : 1;
			uint32_t                           : 8;
			uint32_t block_set_address_request : 1;
			uint32_t trb_type                  : 6;
			uint32_t                           : 8;
			uint32_t slot_id                   : 8;
		} address_device;

		struct
		{
			uint64_t           : 64;
			uint32_t           : 32;
			uint32_t cycle_bit : 1;
			uint32_t           : 9;
			uint32_t trb_type  : 6;
			uint32_t           : 8;
			uint32_t slot_id   : 8;
		} disable_slot_command;

		struct 
		{
			uint64_t cmd_trb_ptr;

			uint32_t completion_param : 24;
			uint8_t  completion_code;

			uint32_t cycle_bit  : 1;
        	uint32_t rsvd1      : 9;
        	uint32_t type    	: 6;
        	uint32_t vf_id      : 8;
        	uint32_t slot_id    : 8;
		} cmd_completion;

		struct
		{
			uint64_t trb_pointer         : 64;

			uint32_t trb_transfer_length : 24;
			uint32_t completion_code     : 8;

			uint32_t cycle_bit           : 1;
			uint32_t                     : 1;
			uint32_t event_data          : 1;
			uint32_t                     : 7;
			uint32_t trb_type            : 6;
			uint32_t endpoint_id         : 5;
			uint32_t                     : 3;
			uint32_t slot_id             : 8;
		} transfer_event;

		struct
		{
			struct xhci_device_request 		 req;

			uint32_t trb_transfer_length     : 17;
			uint32_t                         : 5;
			uint32_t interrupt_target        : 10;

			uint32_t cycle_bit               : 1;
			uint32_t                         : 4;
			uint32_t interrupt_on_completion : 1;
			uint32_t immediate_data          : 1;
			uint32_t                         : 3;
			uint32_t trb_type                : 6;
			uint32_t transfer_type           : 2;
			uint32_t                         : 14;
		} setup_stage;

		struct
		{
			uint64_t data_buffer       		   : 64;

			uint32_t trb_transfer_length       : 17;
			uint32_t td_size                   : 5;
			uint32_t interrupt_target          : 10;

			uint32_t cycle_bit                 : 1;
			uint32_t evaluate_next_trb         : 1;
			uint32_t interrupt_on_short_packet : 1;
			uint32_t no_snoop                  : 1;
			uint32_t chain_bit                 : 1;
			uint32_t interrupt_on_completion   : 1;
			uint32_t immediate_data            : 1;
			uint32_t                           : 3;
			uint32_t trb_type                  : 6;
			uint32_t direction                 : 1;
			uint32_t                           : 15;
		} data_stage;

		struct
		{
			uint32_t rsvd0                   : 32;
			uint32_t rsvd1                   : 32;

			uint32_t                         : 22;
			uint32_t interrupter_target      : 10;

			uint32_t cycle_bit               : 1;
			uint32_t evaluate_next_trb       : 1;
			uint32_t                         : 2;
			uint32_t chain_bit               : 1;
			uint32_t interrupt_on_completion : 1;
			uint32_t                         : 4;
			uint32_t trb_type                : 6;
			uint32_t direction               : 1;
			uint32_t                         : 15;
		} status_stage;
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
		uint8_t target; // 0 for host command ring
		uint8_t rsvdZ;
		uint16_t stream_id; // Should be 0 for host controller doorbells
	} PACKED;
} PACKED;

struct xhci_command_ring
{
	size_t trbs_num;
	size_t next_trb_index;
	
	struct xhci_trb* 	trbs;
	uintptr_t 			trbs_phys;

	struct xhci_trb*	completions;

	uint8_t	cycle_bit;
};

struct xhci_command_ring *xhci_create_command_ring(size_t ntrbs);
size_t xhci_command_enqueue(struct xhci_command_ring *ring, struct xhci_trb* trb);

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
void xhci_interrupter_upd_erdp(struct xhci_interrupter *intr, struct xhci_event_ring *ring);

struct xhci_transfer_ring
{
	size_t trb_count;

	size_t enqueue_ptr;
	size_t dequeue_ptr;

	struct xhci_trb* 	trbs;
	uintptr_t 			trbs_phys;

	uint8_t 	cycle_bit;
};
struct xhci_transfer_ring *xhci_create_transfer_ring(size_t ntrbs);
void xhci_free_transfer_ring(struct xhci_transfer_ring *ring);
void xhci_transfer_ring_enqueue(struct xhci_transfer_ring *ring, struct xhci_trb* trb);
uintptr_t xhci_transfer_ring_get_cur_phys_ptr(struct xhci_transfer_ring *ring);

struct xhci_device;

struct xhci_port_desc
{
	uint8_t revision_major;
	uint8_t revision_minor;
	uint8_t slot_id;

	struct xhci_protocol_cap* 	proto_cap;
	struct xhci_port_regs*  	port_regs;
	struct xhci_device* 		bound_device;
	
	int status_changed; // flag
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

	uint8_t slots;
	uint8_t ports_num;
	uint16_t interrupters;
	uint32_t max_scratchpad_buffers;
	uint32_t pagesize;
	uint16_t ext_cap_offset;
	uint8_t context_size; // 1 - 64bit, 0 - 32 

	struct xhci_command_ring *cmdring;
	struct xhci_event_ring* event_ring;

	// Структуры по портам
	struct xhci_port_desc *ports;

	// Указатели на устройства по номерам слотов
	struct xhci_device** devices_by_slots;

	uintptr_t* dcbaa;

	int port_status_changed;
};

int xhci_controller_enqueue_cmd_wait(struct xhci_controller* controller, struct xhci_command_ring *ring, struct xhci_trb* trb, struct xhci_trb* result);

void xhci_controller_stop(struct xhci_controller* controller);
int xhci_controller_reset(struct xhci_controller* controller);
int xhci_controller_start(struct xhci_controller* controller);
int xhci_controller_check_ext_caps(struct xhci_controller* controller);
int xhci_controller_init_scratchpad(struct xhci_controller* controller);
int xhci_controller_init_interrupts(struct xhci_controller* controller, struct xhci_event_ring* event_ring);
void xhci_controller_process_event(struct xhci_controller* controller, struct xhci_trb* event);
void xhci_controller_init_ports(struct xhci_controller* controller);

struct xhci_device {
	struct xhci_controller* controller;
	uint8_t port_id;
	uint8_t slot_id;
	uint8_t port_speed;
	uint8_t ctx_size;

	// Input Context
	void* input_ctx;
	uintptr_t input_ctx_phys;

	// Output Context
	void* device_ctx;
	uintptr_t device_ctx_phys;

	// Указатели на базовые структуры Input Context
	struct xhci_input_control_context32* input_control_context;
	struct xhci_slot_context32* slot_ctx;
	struct xhci_endpoint_context32* control_endpoint_ctx;

	struct xhci_trb control_completion_trb;
	struct xhci_transfer_ring* control_transfer_ring;
};
struct xhci_device* new_xhci_device(struct xhci_controller* controller, uint8_t port_id, uint8_t slot_id);
void xhci_free_device(struct xhci_device* dev);
int xhci_device_init_contexts(struct xhci_device* dev);
void xhci_device_configure_control_endpoint_ctx(struct xhci_device* dev, uint16_t max_packet_size);
int xhci_device_send_usb_request(struct xhci_device* dev, struct xhci_device_request* req, void* out, uint32_t length);
int xhci_device_get_descriptor(struct xhci_device* dev, struct usb_device_descriptor* descr, uint32_t length);
int xhci_device_get_string_language_descriptor(struct xhci_device* dev, struct usb_string_language_descriptor* descr);
int xhci_device_get_string_descriptor(struct xhci_device* dev, uint16_t language_id, uint8_t index, struct usb_string_descriptor* descr);
int xhci_device_get_configuration_descriptor(struct xhci_device* dev, uint8_t configuration, struct usb_configuration_descriptor* descr, size_t buffer_size);
int xhci_device_set_configuration(struct xhci_device* dev, uint8_t configuration);
int xhci_device_handle_transfer_event(struct xhci_device* dev, struct xhci_trb* event);

/// @brief 
/// @param controller указатель на объект контроллера xhci
/// @param port_id номер порта (0-255)
/// @return TRUE при успехе включения, или если устройство уже было включено 
int xhci_controller_poweron(struct xhci_controller* controller, uint8_t port_id);

/// @brief 
/// @param controller указатель на объект контроллера xhci
/// @param port_id номер порта (0-255)
/// @return TRUE при успехе сброса порта
int xhci_controller_reset_port(struct xhci_controller* controller, uint8_t port_id);

/// @brief 
/// @param controller указатель на объект контроллера xhci
/// @param port_id номер порта (0-255)
/// @return TRUE при успехе инициализации устройства
int xhci_controller_init_device(struct xhci_controller* controller, uint8_t port_id);

uint8_t xhci_controller_alloc_slot(struct xhci_controller* controller);
int xhci_controller_disable_slot(struct xhci_controller* controller, uint8_t slot);
int xhci_controller_address_device(struct xhci_controller* controller, uintptr_t address, uint8_t slot_id, int bsr);

// Общий обработчик прерывания
void xhci_int_hander(void* regs, struct xhci_controller* data);
void xhci_controller_event_thread_routine(struct xhci_controller* controller);

#endif