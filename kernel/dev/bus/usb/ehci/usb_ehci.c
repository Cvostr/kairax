#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/device_man.h"

int ehci_device_probe(struct device *dev) 
{
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