#ifndef _USB_XHCI_H
#define _USB_XHCI_H

#include "kairax/types.h"

#define XHCI_CMD_RUN        (1 << 0)
#define XHCI_CMD_RESET      (1 << 1)

#define XHCI_STS_HALT       (1 << 0)
#define XHCI_STS_NOT_READY  (1 << 11)

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

struct xhci_op_regs {
	volatile uint32_t usbcmd;
	volatile uint32_t usbsts;   // 4
	volatile uint32_t pagesize; // 8h
	volatile uint32_t pad1[2]; // ch 10h
	volatile uint32_t dnctrl;   // 14h
	volatile uint32_t crcr[2];     // 18h 1ch
	volatile uint32_t pad2[4]; // 20h 24h 28h 2ch
	volatile uint32_t dcbaap[2];   // 30h 34h
	volatile uint32_t config;   // 38h
	volatile uint8_t  padding3[964]; // 3ch-400h
	struct xhci_port_regs portregs[256];
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
};

void xhci_int_hander();

#endif