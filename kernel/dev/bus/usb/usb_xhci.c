#include "dev/bus/pci/pci.h"
#include "usb.h"
#include "usb_xhci.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
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

	// Получение свойств
	cntrl->slots = cntrl->cap->hcsparams1 & 0xFF;
	cntrl->ports = cntrl->cap->hcsparams1 >> 24;
	cntrl->interrupters = (cntrl->cap->hcsparams1 >> 8) & 0b11111111111;
	printk("XHCI: version: %i, slots: %i, ports: %i, ints: %i\n", (cntrl->cap->version) & 0xFFFF, cntrl->slots, cntrl->ports, cntrl->interrupters);

	// Остановка и сброс контроллера
	xhci_controller_stop(cntrl);
	xhci_controller_reset(cntrl);

	uint16_t max_scratchpad_hi = (cntrl->cap->hcsparams2 >> 20) & 0b11111;
	uint16_t max_scratchpad_lo = (cntrl->cap->hcsparams2 >> 27) & 0b11111;
	cntrl->max_scratchpad_buffers = max_scratchpad_hi << 5 | max_scratchpad_lo;
	cntrl->ext_cap_offset = (cntrl->cap->hccparams1 >> 16) & UINT16_MAX;
	cntrl->pagesize = 1ULL << (cntrl->op->pagesize + 12);
	printk("XHCI: scratchpad: %i, pagesize: %i, ext cap offset: %i\n", cntrl->max_scratchpad_buffers, cntrl->pagesize, cntrl->ext_cap_offset);

	if (cntrl->ext_cap_offset == 0)
	{
		printk("XHCI: Controller doesnt have extended capabilities\n");
		unmap_io_region(cntrl->mmio_addr_phys, dev->pci_info->BAR[0].size);
		kfree(cntrl);
		return -1;
	}

	xhci_controller_init_ports(cntrl);

	// DCBAA хранит адреса на контекстную структуру каждого слота
	size_t dcbaa_size = cntrl->slots * sizeof(void*);
	// Выделить память
	uintptr_t dcbaa_phys = (uintptr_t) pmm_alloc(dcbaa_size, NULL);
	cntrl->dcbaa = map_io_region(dcbaa_phys, dcbaa_size);
	// Записать адрес в регистр DCBAA
	cntrl->op->dcbaa_h = dcbaa_phys >> 32;
	cntrl->op->dcbaa_l = dcbaa_phys & UINT32_MAX;

	// Выделить память под CRCR с TRB
	size_t crcr_size = 256 * sizeof(struct xhci_trb);
	// Выделить память
	uintptr_t crcr_phys = (uintptr_t) pmm_alloc(crcr_size, NULL);
	cntrl->crcr = map_io_region(crcr_phys, crcr_size);
	// Записать адрес в регистр CRCR
	cntrl->op->crcr_h = crcr_phys >> 32;
	cntrl->op->crcr_l = (crcr_phys & UINT32_MAX) | XHCI_CR_CYCLE_STATE;
	

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

void xhci_controller_stop(struct xhci_controller* controller)
{
	uint32_t cmd = controller->op->usbcmd;
	cmd &= ~XHCI_CMD_RUN;
	controller->op->usbcmd = cmd;

	while (!(controller->op->usbsts & XHCI_STS_HALT));
}

void xhci_controller_reset(struct xhci_controller* controller)
{
	uint32_t cmd = controller->op->usbcmd;
	cmd |= XHCI_CMD_RESET;
	controller->op->usbcmd = cmd;

	while ((controller->op->usbcmd & XHCI_CMD_RESET));
	while ((controller->op->usbsts & XHCI_STS_NOT_READY));
}

int xhci_controller_init_ports(struct xhci_controller* controller)
{
	char* ext_cap_ptr = controller->mmio_addr + controller->ext_cap_offset * sizeof(uint32_t);

	while (1)
	{
		struct xhci_extended_cap* cap = (struct xhci_extended_cap*) ext_cap_ptr;

		if (cap->capability_id == XHCI_EXT_CAP_SUPPORTED_PROTOCOL)
		{
			struct xhci_protocol_cap* prot_cap = (struct xhci_protocol_cap*) ext_cap_ptr;
			const uint32_t target_name_string =
				('U' <<  0) |
				('S' <<  8) |
				('B' << 16) |
				(' ' << 24);

			if (prot_cap->name_string != target_name_string)
			{
				printk("Invalid port protocol name string\n");
				return -1;
			}
		}

		if (cap->next_capability > 0)
		{
			ext_cap_ptr += cap->next_capability * sizeof(uint32_t);
		} 
		else
		{
			break;
		}
	}

	controller->op->config.max_device_slots_enabled = controller->slots;

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
