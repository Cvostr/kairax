#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "kairax/string.h"
#include "rtl8169.h"

int rtl8169_device_probe(struct device *dev) 
{
	pci_set_command_reg(dev->pci_info, pci_get_command_reg(dev->pci_info) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
    pci_device_set_enable_interrupts(dev->pci_info, 1);

	struct rtl8169* rtl_dev = kmalloc(sizeof(struct rtl8169));
    memset(rtl_dev, 0, sizeof(struct rtl8169));

	// Сохранение BAR0
	rtl_dev->io_addr = (uint32_t) dev->pci_info->BAR[0].address;

	// "Сброс" устройства 
	int rc = rtl8169_reset(rtl_dev);
	if (rc != 0)
	{
		printk("RTL8169: Reset failed\n");
		return -1;
	}

	// Чтение MAC во внутренний массив
	rtl8169_read_mac(rtl_dev);

	printk ("RTL8169: MAC %x:%x:%x:%x:%x:%x\n", 
		rtl_dev->mac[0], 
		rtl_dev->mac[1],
		rtl_dev->mac[2],
		rtl_dev->mac[3],
		rtl_dev->mac[4],
		rtl_dev->mac[5]);

	rtl8169_lock_unlock_9346(rtl_dev, TRUE);

	// TODO: инициализировать всё


	rtl8169_lock_unlock_9346(rtl_dev, FALSE);

	// Регистрация интерфейса в ядре
    rtl_dev->nic = new_nic();
    memcpy(rtl_dev->nic->mac, rtl_dev->mac, MAC_DEFAULT_LEN);
    rtl_dev->nic->dev = dev; 
    rtl_dev->nic->flags = NIC_FLAG_UP | NIC_FLAG_BROADCAST | NIC_FLAG_MULTICAST;
    //rtl_dev->nic->tx = rtl8139_tx;
    //rtl_dev->nic->up = rtl8139_up;
    //rtl_dev->nic->down = rtl8139_down;
    rtl_dev->nic->mtu = 1500; // уточнить
    register_nic(rtl_dev->nic, "eth");

    dev->dev_type = DEVICE_TYPE_NETWORK_ADAPTER;
    dev->dev_data = rtl_dev;
    dev->nic = rtl_dev->nic;

	return 0;
}

int rtl8169_reset(struct rtl8169* rtl_dev)
{
	// reset
	outb(rtl_dev->io_addr + RTL8169_CR, RTL8169_CR_RST);

	while (inb(rtl_dev->io_addr + RTL8169_CR) & RTL8169_CR_RST)
	{

	}

	return 0;
}

void rtl8169_read_mac(struct rtl8169* rtl_dev)
{
	for (uint32_t i = 0; i < 6; i++)
	{
		rtl_dev->mac[i] = inb(rtl_dev->io_addr + i); /*ioaddr is the base address obtainable from the PCI device configuration space.*/
	}
}

void rtl8169_lock_unlock_9346(struct rtl8169* rtl_dev, int unlock)
{
	uint8_t val = unlock == TRUE ? RTL8169_9346_CONFIG : RTL8169_9346_NORMAL;
	outb(rtl_dev->io_addr + RTL8169_9346CR, val);
}

struct device_driver_ops rtl8169_ops = {
    .probe = rtl8169_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_id rtl8169_pci_id[] = {
	{ PCI_DEVICE(REALTEK_PCIID, 0x8161) },
	{ PCI_DEVICE(REALTEK_PCIID, 0x8168) },
	{ PCI_DEVICE(REALTEK_PCIID, 0x8169) },
	{0,}
};

struct pci_device_driver rtl8169_driver = {
	.dev_name = "Realtek RTL8169 Ethernet Adapter",
	.pci_device_ids = rtl8169_pci_id,
	.ops = &rtl8169_ops
};

int module_init(void)
{
    register_pci_device_driver(&rtl8169_driver);
	
	return 0;
}

void module_exit(void)
{

}

DEFINE_MODULE("rtl8169", module_init, module_exit)