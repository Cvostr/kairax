#include "dev/bus/pci/pci.h"
#include "usb.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/interrupts.h"

int xhci_device_probe(struct device *dev) 
{
	pci_set_command_reg(dev->pci_info, pci_get_command_reg(dev->pci_info) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
    pci_device_set_enable_interrupts(dev->pci_info, 1);

    return 0;
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
