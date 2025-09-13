#include "usb_ehci.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/device_man.h"
#include "mem/iomem.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "string.h"

int ehci_device_probe(struct device *dev) 
{
	int rc;
	struct pci_device_info* device_desc = dev->pci_info;

	printk("EHCI controller found on bus: %i, device: %i func: %i\n", 
		device_desc->bus,
		device_desc->device,
		device_desc->function);

	struct pci_device_bar* BAR = &device_desc->BAR[0];
	if (BAR->type != BAR_TYPE_MMIO)
	{
		printk("EHCI: Controller BAR is not MMIO %i\n");
		return -1;
	}

	int msi_capable = pci_device_is_msi_capable(dev->pci_info);
	int msix_capable = pci_device_is_msix_capable(dev->pci_info);
	printk("EHCI: MSI capable: %i MSI-X capable: %i\n", msi_capable, msix_capable);

	pci_set_command_reg(dev->pci_info, 
	pci_get_command_reg(dev->pci_info) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE | ~PCI_DEVCMD_IO_ENABLE);
    pci_device_set_enable_interrupts(dev->pci_info, 1);

	struct ehci_controller* cntrl = kmalloc(sizeof(struct ehci_controller));
	memset(cntrl, 0, sizeof(struct ehci_controller));

	// Сохраним указатель на объект устройства в системе
	cntrl->controller_dev = dev;
	// Разметить и сохранить BAR
	cntrl->mmio_addr_phys = BAR->address;
    cntrl->mmio_addr = map_io_region(cntrl->mmio_addr_phys, BAR->size);

	cntrl->cap_base = cntrl->mmio_addr;
	// Считаем CAPLEN
	uint8_t caplen = cntrl->cap_base->caplength;
	printk("EHCI: CAPLEN %i\n", caplen);
	cntrl->op_base = cntrl->mmio_addr + caplen;

	// ВЫделить память под frame list
	cntrl->frame_list_phys = pmm_alloc(PAGE_SIZE, NULL);
	cntrl->frame_list = map_io_region(cntrl->frame_list_phys, PAGE_SIZE);
	memset(cntrl->frame_list, 0, PAGE_SIZE);

    return 0;
}

struct pci_device_id ehci_ctrl_ids[] = {
	{PCI_DEVICE_CLASS(0xC, 0x3, 0x20)},
	{0,}
};

struct device_driver_ops ehci_ctrl_ops = {

    .probe = ehci_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_driver ehci_ctrl_driver = {
	.dev_name = "USB 2.0 (EHCI) Host Controller",
	.pci_device_ids = ehci_ctrl_ids,
	.ops = &ehci_ctrl_ops
};