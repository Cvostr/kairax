#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "kairax/string.h"
#include "e1000.h"

int e1000_device_probe(struct device *dev) 
{
	struct e1000* e1000_dev = kmalloc(sizeof(struct e1000));
    memset(e1000_dev, 0, sizeof(struct e1000));
    e1000_dev->dev = dev;

	// Запомним объект BAR
	e1000_dev->BAR = &dev->pci_info->BAR[0];
	if (e1000_dev->BAR->type == BAR_TYPE_MMIO)
	{
		e1000_dev->mmio = map_io_region(e1000_dev->BAR->address, e1000_dev->BAR->size);
	}

	e1000_detect_eeprom(e1000_dev);
	printk("E1000: EEPROM Detected %i\n", e1000_dev->has_eeprom);

	int rc = e1000_get_mac(e1000_dev);
	if (rc != 0)
	{
		printk("E1000: Error getting MAC address %i\n", rc);
		return -1;
	}

	printk ("E1000: MAC %x:%x:%x:%x:%x:%x\n", 
		e1000_dev->mac[0], 
		e1000_dev->mac[1],
		e1000_dev->mac[2],
		e1000_dev->mac[3],
		e1000_dev->mac[4],
		e1000_dev->mac[5]);

	return -1;
}

void e1000_detect_eeprom(struct e1000* dev)
{
	dev->has_eeprom = FALSE;
	e1000_write32(dev, E1000_REG_EEPROM, 0x01);

	for (int i = 0; i < 1000; i++)
	{
		if (e1000_read32(dev, E1000_REG_EEPROM) & 0x10)
		{
			dev->has_eeprom = TRUE;
			break;
		}
	}
}

uint16_t e1000_eeprom_read(struct e1000* dev, uint8_t addr)
{
	uint32_t tmp = 0;
	if (dev->has_eeprom == TRUE)
	{
		e1000_write32(dev, E1000_REG_EEPROM, ((uint32_t)addr << 8) | 1);
		while (!((tmp = e1000_read32(dev, E1000_REG_EEPROM)) & (1 << 4)))
		{

		}
	}
	else
	{
		e1000_write32(dev, E1000_REG_EEPROM, ((uint32_t)addr << 2) | 1);
		while (!((tmp = e1000_read32(dev, E1000_REG_EEPROM)) & (1 << 1)))
		{

		}
	}

	return (tmp >> 16) & 0xFFFF;
}

int e1000_get_mac(struct e1000* dev)
{
	if (dev->has_eeprom == TRUE)
	{
		uint32_t temp = e1000_eeprom_read(dev, 0);
		dev->mac[0] = temp;
		dev->mac[1] = temp >> 8;

		temp = e1000_eeprom_read(dev, 1);
		dev->mac[2] = temp;
		dev->mac[3] = temp >> 8;

		temp = e1000_eeprom_read(dev, 2);
		dev->mac[4] = temp;
		dev->mac[5] = temp >> 8;

		return 0;
	} 

	printk("E1000: Implement MAC without EEPROM\n");

	return 1;
}

int e1000_init_rx(struct e1000* dev)
{
	return 0;
}

int e1000_init_tx(struct e1000* dev)
{
	return 0;
}

void e1000_write32(struct e1000* dev, off_t offset, uint32_t value)
{
	if (dev->BAR->type == BAR_TYPE_IO)
	{
		outl(dev->BAR->address + offset, value);
	} 
	else
	{
		volatile uint32_t* addr = (dev->mmio + offset);
		*addr = value;
	}
}

uint32_t e1000_read32(struct e1000* dev, off_t offset)
{
	if (dev->BAR->type == BAR_TYPE_IO)
	{
		return inl(dev->BAR->address + offset);
	} 

	volatile uint32_t* addr = (dev->mmio + offset);
	return *addr;
}

struct device_driver_ops e1000_ops = {
    .probe = e1000_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_id e1000_pci_id[] = {
	{ PCI_DEVICE(INTEL_PCIID, DEVID_EMULATED) },
	{ PCI_DEVICE(INTEL_PCIID, DEVID_I217) },
	{0,}
};

struct pci_device_driver e1000_driver = {
	.dev_name = "Intel E1000 Ethernet Adapter",
	.pci_device_ids = e1000_pci_id,
	.ops = &e1000_ops
};

int module_init(void)
{
    register_pci_device_driver(&e1000_driver);
	
	return 0;
}

void module_exit(void)
{

}

DEFINE_MODULE("e1000", module_init, module_exit)