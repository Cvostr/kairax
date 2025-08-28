#ifndef _USB_EHCI_H
#define _USB_EHCI_H

#include "kairax/types.h"
#include "../usb_descriptors.h"
#include "../usb.h"

struct ehci_cap_reg {
    volatile uint8_t caplength;
    volatile uint8_t reserved;
    volatile uint32_t hcsparams;
    volatile uint32_t hccparams;
    volatile uint32_t hcsp_portroute;
} PACKED;

#define EHCI_REG_OP_USBCMD              0x00
#define EHCI_REG_OP_USBSTS              0x04
#define EHCI_REG_OP_USBINTR             0x08
#define EHCI_REG_OP_FRINDEX             0x0C
#define EHCI_REG_OP_CTRLDSSEGMENT       0x10
#define ECHI_REG_OP_PERIODICLISTBASE    0x14
#define EHCI_REG_OP_ASYNCLISTADDR       0x18
#define EHCI_REG_OP_CONFIGFLAG          0x40
#define EHCI_REG_OP_PORTSC              0x44

struct ehci_op_reg {
    volatile uint32_t usbcmd;
    volatile uint32_t usbsts;
    volatile uint32_t usbintr;
    volatile uint32_t frindex;
    volatile uint32_t ctrldssegment;
    volatile uint32_t periodiclistbase;
    volatile uint32_t asynclistaddr;
    char padding[40];
    volatile uint32_t configflag;
};  

struct ehci_controller 
{
	struct device* controller_dev;
	// Основной BAR
    char* mmio_addr;
    uintptr_t mmio_addr_phys;
    // Смещения
    struct ehci_cap_reg* cap_base;
    struct ehci_op_reg* op_base;
};

#endif