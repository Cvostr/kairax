#include "dev/bus/pci/pci.h"
#include "usb.h"
#include "usb_xhci.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "mem/kheap.h"
#include "string.h"
#include "stdio.h"
#include "kstdlib.h"

int xhci_device_probe(struct device *dev) 
{
	struct pci_device_info* device_desc = dev->pci_info;

	printk("XHCI controller found on bus: %i, device: %i func: %i\n", 
		device_desc->bus,
		device_desc->device,
		device_desc->function);

	pci_set_command_reg(dev->pci_info, 
		pci_get_command_reg(dev->pci_info) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE | ~PCI_DEVCMD_IO_ENABLE);
    pci_device_set_enable_interrupts(dev->pci_info, 1);

	if (pci_device_is_msi_capable(dev->pci_info) == 0) 
	{
		printk("XHCI: Controller does not support MSI interrupts\n");
		return -1;
	}

	struct xhci_controller* cntrl = kmalloc(sizeof(struct xhci_controller));
	memset(cntrl, 0, sizeof(struct xhci_controller));

	cntrl->mmio_addr_phys = dev->pci_info->BAR[0].address;

	if (cntrl->mmio_addr_phys == NULL)
	{
		printk("XHCI: Controller BAR0 is NULL\n");
		return -1;
	}

    cntrl->mmio_addr = map_io_region(cntrl->mmio_addr_phys, dev->pci_info->BAR[0].size);

	// указатели на основные структуры
	cntrl->cap = (struct xhci_cap_regs*) cntrl->mmio_addr;
	cntrl->op = (struct xhci_op_regs*) (cntrl->mmio_addr + (cntrl->cap->caplen_version & 0xFF));

	cntrl->slots = cntrl->cap->hcsparams1 & 0xFF;
	cntrl->ports = cntrl->cap->hcsparams1 >> 24;
	printk("XHCI: version: %i, slots: %i, ports: %i\n", (cntrl->cap->version) & 0xFFFF, cntrl->slots, cntrl->ports);

	uint32_t cmd = cntrl->op->usbcmd;
	cmd &= ~XHCI_CMD_RUN;
	cntrl->op->usbcmd = cmd;

	while (!(cntrl->op->usbsts & XHCI_STS_HALT));

	cmd = cntrl->op->usbcmd;
	cmd |= XHCI_CMD_RESET;
	cntrl->op->usbcmd = cmd;

	while ((cntrl->op->usbcmd & XHCI_CMD_RESET));
	while ((cntrl->op->usbsts & XHCI_STS_NOT_READY));

	uint16_t max_scratchpad_hi = (cntrl->cap->hcsparams2 >> 20) & 0b11111;
	uint16_t max_scratchpad_lo = (cntrl->cap->hcsparams2 >> 27) & 0b11111;
	cntrl->max_scratchpad_buffers = max_scratchpad_hi << 5 | max_scratchpad_lo;

	cntrl->pagesize = 1ULL << (cntrl->op->pagesize + 12);
	printk("XHCI: scratchpad: %i, pagesize: %i\n", cntrl->cap->hcsparams2, cntrl->max_scratchpad_buffers, cntrl->pagesize);

	uint8_t irq = alloc_irq(0, "xhci");
    printk("XHCI: using IRQ %i\n", irq);
    int rc = pci_device_set_msi_vector(dev, irq);
    if (rc == -1) {
        printk("Error: Device doesn't support MSI\n");
        return -1;
    }
	register_irq_handler(irq, xhci_int_hander, cntrl);

    return 0;
}

void xhci_int_hander() 
{
	printk("XHCI: interrupt\n");
}

struct pci_device_id xhci_ctrl_ids[] = {
	{PCI_DEVICE_CLASS(0xC, 0x3, 0x30)},
	{0,}
};

struct device_driver_ops usb3_ctrl_ops = {

    .probe = xhci_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_driver xhci_ctrl_driver = {
	.dev_name = "USB 3.0 (XHCI) Host Controller",
	.pci_device_ids = xhci_ctrl_ids,
	.ops = &usb3_ctrl_ops
};

void usb_init()
{
	register_pci_device_driver(&xhci_ctrl_driver);

}
