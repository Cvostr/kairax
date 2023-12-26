#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "hda.h"
#include "mem/iomem.h"

void hda_outd(struct hda_dev* dev, uintptr_t base, uint32_t value) 
{
    uint32_t* addr = (uint32_t*) (dev->io_addr + base);
    *addr = value;
}

uint32_t hda_ind(struct hda_dev* dev, uintptr_t base) 
{
    uint32_t* addr = (uint32_t*) (dev->io_addr + base);
    return *addr;
}

int hda_device_probe(struct device *dev) 
{
    printk("Intel HD Audio Device found\n");

    struct hda_dev* dev_data = kmalloc(sizeof(struct hda_dev));
    dev_data->io_addr = map_io_region(dev->pci_info->BAR[0].address, dev->pci_info->BAR[0].size);

    dev->dev_data = dev_data;

    hda_outd(dev_data, CRST, 1);
    while (hda_ind(dev_data, CRST) != 0x1);

    printk("HDA device initialized\n");

    return 0;
}

struct pci_device_id hda_pci_id[] = {
	{PCI_DEVICE_CLASS(0x04, 0x03, 0x00)},
	{0,}
};

struct device_driver_ops hda_ops = {

    .probe = hda_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_driver hda_driver = {
	.dev_name = "Intel HD Audio Controller",
	.pci_device_ids = hda_pci_id,
	.ops = &hda_ops
};

int init(void)
{
	printk("Intel HD Audio driver\n");
    register_pci_device_driver(&hda_driver);
	
	return 0;
}

void exit(void)
{

}

DEFINE_MODULE("hda", init, exit)