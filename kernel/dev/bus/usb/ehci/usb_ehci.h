#ifndef _USB_EHCI_H
#define _USB_EHCI_H

#include "kairax/types.h"
#include "../usb_descriptors.h"
#include "../usb.h"

// CAP REGS
#define EHCI_REG_CAPLENGTH              0x00
#define EHCI_REG_HCIVERSION             0x02
#define EHCI_REG_HCSPARAMS              0x04
#define EHCI_REG_HCCPARAMS              0x08
#define EHCI_REG_HCSP_PORTROUTE         0x0C

#define EHCI_HCCPARAMS_64BIT            0x1

#define EHCI_HCSPARAMS_NPORTS_MASK      0x0F
#define EHCI_HCSPARAMS_PPC              0x10

// OP REGS OFFSETS
#define EHCI_REG_OP_USBCMD              0x00
#define EHCI_REG_OP_USBSTS              0x04
#define EHCI_REG_OP_USBINTR             0x08
#define EHCI_REG_OP_FRINDEX             0x0C
#define EHCI_REG_OP_CTRLDSSEGMENT       0x10
#define ECHI_REG_OP_PERIODICLISTBASE    0x14
#define EHCI_REG_OP_ASYNCLISTADDR       0x18
#define EHCI_REG_OP_CONFIGFLAG          0x40
#define EHCI_REG_OP_PORTSC              0x44

// USBCMD
#define EHCI_USBCMD_RS                  0x001ULL
#define EHCI_USBCMD_HCRESET             0x002
#define EHCI_USBCMD_PERIODIC_ENABLE     0x010
#define EHCI_USBCMD_ASYNC_ENABLE        0x020
#define EHCI_USBCMD_INT_ON_AAB          0x040   // Interrupt on Async Advance Doorbell
#define EHCI_USBCMD_LIGHT_RESET         0x080   // Light Host Controller Reset
#define EHCI_USBCMD_ITC_SHIFT           16

// USBSTS
#define EHCI_USBSTS_USBINT              0x1
#define EHCI_USBSTS_USBERRINT           0x2
#define EHCI_USBSTS_PORT_CHANGE_DETECT  0x4
#define EHCI_USBSTS_FRAME_LIST_ROLLOVER 0x8
#define EHCI_USBSTS_HOST_SYSTEM_ERROR   0x10
#define EHCI_USBSTS_INT_ON_ASYNC_ADV    0x20
#define EHCI_USBSTS_HCHALTED            0x1000

// USBINTR
#define EHCI_USBINTR_INT_ENABLE         0x1
#define EHCI_USBINTR_ERR_INT_ENABLE     0x2
#define EHCI_USBINTR_PORT_CHANGE_INT    0x4
#define EHCI_USBINTR_FLR                0x8
#define EHCI_USBINTR_HSE                0x10
#define EHCI_USBINTR_AADV               0x20

// PORTSC
#define EHCI_PORTSC_CONNECT_STATUS          0x1
#define EHCI_PORTSC_CONNECT_STATUS_CHANGE   0x2
#define EHCI_PORTSC_ENABLED                 0x4
#define EHCI_PORTSC_ENABLED_STATUS_CHANGE   0x8
#define EHCI_PORTSC_OVER_CURRENT_ACTIVE     0x10
#define EHCI_PORTSC_OVER_CURRENT_CHANGE     0x20
#define EHCI_PORTSC_FORCE_PORT_RESUME       0x40
#define EHCI_PORTSC_SUSPEND                 0x80
#define EHCI_PORTSC_PORT_RESET              0x100
#define EHCI_PORTSC_LINE_STATUS_MASK        0xC00
#define EHCI_PORTSC_PORT_POWER              0x1000
#define EHCI_PORTSC_PORT_OWNER              0x2000

#define EHCI_PORTSC_LINE_STATUS_K           0b01

#define EHCI_TYPE_I_TD      0b00
#define EHCI_TYPE_QH        0b01
#define EHCI_TYPE_SI_TD     0b10
#define EHCI_TYPE_FSTN      0b11

#define EHCI_USBLEGSUP      0x00
#define EHCI_USBLEGCTLSTS   0x04
#define USBLEGSUP_HC_BIOS               0x10000     // BIOS Owned Semaphore
#define USBLEGSUP_HC_OS                 0x1000000   // OS Owned Semaphore

#define EHCI_PID_CODE_OUT       0b00
#define EHCI_PID_CODE_IN        0b01
#define EHCI_PID_CODE_SETUP     0b10

#define EHCI_EP_SPEED_FULL_SPEED    0b00    // 12Mbs
#define EHCI_EP_SPEED_LOW_SPEED     0b01    // 1.5Mbs
#define EHCI_EP_SPEED_HIGH_SPEED    0b10    // 480Mbs
#define EHCI_EP_SPEED_RESERVED      0b11

struct ehci_flp 
{
    uint32_t terminate : 1;           // Terminate
    uint32_t type : 2;                // Type
    uint32_t reserved : 2;            // Reserved
    uint32_t lp : 27;                 // Link pointer
} PACKED;

// Queue Element Transfer Descriptor
// EHCI Revision 1.0 страница 41
struct ehci_td 
{
    // Next qTD Pointer
    struct {
        uint32_t terminate : 1;
        uint32_t reserved : 4;
        uint32_t lp : 27;
    } next;

    // Alternate Next qTD Pointer
    struct {
        uint32_t terminate : 1;
        uint32_t reserved : 4;
        uint32_t lp : 27;
    } alt_next;

    struct {
        struct {
            uint32_t ping_state : 1;
            uint32_t split_xstate : 1;
            uint32_t missed : 1;
            uint32_t transaction_error : 1;
            uint32_t babble : 1;
            uint32_t data_buffer_error : 1;
            uint32_t halted : 1;
            uint32_t active : 1;
        } PACKED status;

        uint32_t pid_code : 2;
        uint32_t err_count : 2;
        uint32_t curr_page : 3;
        uint32_t ioc : 1;
        uint32_t total_len : 15;
        uint32_t data_toggle : 1;

    } PACKED token;

    uint32_t buffer_ptr_list[5];

    uint32_t ext_buffer_ptr[5];

} PACKED;

// Queue Head
// EHCI Revision 1.0 страница 46
struct ehci_qh 
{
    // Queue Head Horizontal Link Pointer
    union {
        struct {
            uint32_t terminate : 1;
            uint32_t type : 2;
            uint32_t zero : 2;
            uint32_t lp : 27;
        } PACKED;

        volatile uint32_t raw;
    } PACKED link_ptr;


    // Endpoint Characteristics
    struct 
    {
        uint32_t device_address : 7;
        uint32_t i : 1; // inactivate on next
        uint32_t endpt : 4; // endpoint number
        uint32_t eps : 2; // endpoint speed
        uint32_t dtc : 1; // Data toggle control
        uint32_t h : 1; // Head of Reclamation List Flag
        uint32_t max_pkt_length : 11;
        uint32_t c : 1; // Control Endpoint Flag
        uint32_t rl : 4; // Nak Count Reload
    } ep_ch;

    // Endpoint Capabilities
    struct 
    {
        uint32_t int_schedule_mask : 8;
        uint32_t split_completion_mask : 8;
        uint32_t hub_addr : 7;
        uint32_t port : 7;
        uint32_t mult : 2;
    } ep_cap;

    // Current qTD Pointer
    struct 
    {
        uint32_t rsvd : 5;
        uint32_t ptr : 27;
    } current_ptr;

    // Next qTD Pointer
    struct {
        uint32_t terminate : 1;
        uint32_t reserved : 4;
        uint32_t lp : 27;
    } next;

    // Alternate Next qTD Pointer
    struct {
        uint32_t terminate : 1;
        uint32_t nak_cnt : 4;
        uint32_t lp : 27;
    } alt_next;

    struct {
        struct {
            uint32_t ping_state : 1;
            uint32_t split_xstate : 1;
            uint32_t missed : 1;
            uint32_t transaction_error : 1;
            uint32_t babble : 1;
            uint32_t data_buffer_error : 1;
            uint32_t halted : 1;
            uint32_t active : 1;
        } status;

        uint32_t pid_code : 2;
        uint32_t err_count : 2;
        uint32_t curr_page : 3;
        uint32_t ioc : 1;
        uint32_t total_len : 15;
        uint32_t data_toggle : 1;

    } token;

    struct {
        uint32_t offset_reserved : 12;
        uint32_t buffer_ptr : 20;
    } buffer_ptr_list[5];

    uint32_t extended_buffer_ptr[5];

    // Внутреннее состояние для драйвера
    struct ehci_qh *prev_ptr;
    struct ehci_qh *next_ptr;

} PACKED;

struct ehci_controller 
{
	struct device* controller_dev;
	// Основной BAR
    int bar_type;
    char* mmio_addr;
    uintptr_t mmio_addr_phys;
    // Смещения
    uintptr_t op_offset;
    // кол-во портов
    size_t nports;
    // Поддерживаются ли контроллером 64 битные адреса
    int mode64;

    int port_status_changed;

    uint8_t device_address_counter;

    void* frame_list_phys;
    struct ehci_flp* frame_list;

    // TEMP
    struct ehci_qh* async_head;
    struct ehci_qh* async_tail;
};

struct ehci_device
{
    struct ehci_controller *hci;
    uint8_t port_id;
    uint32_t address;
    uint8_t  speed;
    uint32_t controlEpMaxPacketSize;
};

#define QH_LINK_TERMINATE(qh) (qh->link_ptr.terminate = 1)
#define TD_LINK_TERMINATE(td) {td->next.terminate = 1; td->alt_next.terminate = 1;}

int ehci_allocate_pool(struct ehci_controller *ehc, size_t qh_pool, size_t td_pool);

struct ehci_qh *ehci_alloc_qh(struct ehci_controller *ehc);
void ehci_free_qh(struct ehci_controller *ehc, struct ehci_qh *qh);

struct ehci_td *ehci_alloc_td(struct ehci_controller *ehc);
void ehci_free_td(struct ehci_controller *ehc, struct ehci_td *td);

void ehci_qh_link_qh(struct ehci_qh *qh_parent, struct ehci_qh *qh);
void ehci_qh_link_td(struct ehci_qh *qh, struct ehci_td *td);
void ehci_td_link_td(struct ehci_td *td_parent, struct ehci_td *td);

// Используется для добавления QH в список на исполнение контроллеру
void ehci_enqueue_qh(struct ehci_controller* hci, struct ehci_qh *qh);
// Используется для удаления отработавшего QH
void ehci_dequeue_qh(struct ehci_controller* hci, struct ehci_qh *qh);

uint8_t ehci_read8(struct ehci_controller* cntrl, off_t offset);
uint16_t ehci_read16(struct ehci_controller* cntrl, off_t offset);
uint32_t ehci_read32(struct ehci_controller* cntrl, off_t offset);

uint32_t ehci_op_read32(struct ehci_controller* cntrl, off_t offset);

int ehci_reset(struct ehci_controller* cntrl);

#endif