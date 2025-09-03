#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "kairax/string.h"
#include "e1000.h"

#define E1000_NUM_RX_DESCS 128
#define E1000_NUM_TX_DESCS 64

#define E1000_TX_BUF_SZ		8192
#define E1000_RX_BUF_SZ		8192

#define LOG_DEBUG_MAC

int e1000_device_probe(struct device *dev) 
{
	struct e1000* e1000_dev = kmalloc(sizeof(struct e1000));
    memset(e1000_dev, 0, sizeof(struct e1000));
    e1000_dev->dev = dev;

	pci_set_command_reg(dev->pci_info, pci_get_command_reg(dev->pci_info) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);

	int msi_capable = pci_device_is_msi_capable(dev->pci_info);
	printk("E1000: MSI capable: %i\n", msi_capable);

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

#ifdef LOG_DEBUG_MAC
	printk ("E1000: MAC %x:%x:%x:%x:%x:%x\n", 
		e1000_dev->mac[0], 
		e1000_dev->mac[1],
		e1000_dev->mac[2],
		e1000_dev->mac[3],
		e1000_dev->mac[4],
		e1000_dev->mac[5]);
#endif

	rc = e1000_init_rx(e1000_dev);
	printk("E1000: Rx initialized\n");

	rc = e1000_init_tx(e1000_dev);
	printk("E1000: Tx initialized\n");

	e1000_enable_link(e1000_dev);

	// Добавление обработчика
    int irq = pci_device_get_irq_line(dev->pci_info);
    printk("E1000: IRQ %i\n", irq);
    rc = register_irq_handler(irq, e1000_irq_handler, e1000_dev);
	if (rc != 0)
	{
		printk("E1000: Error registering IRQ Handler %i\n", rc);
	}
	pci_device_set_enable_interrupts(dev->pci_info, 1);

	// Выбор маски прерываний
	e1000_write32(e1000_dev, E1000_REG_IMASK, ICR_RXT0 | ICR_RXSEQ);
	e1000_write32(e1000_dev, E1000_REG_ICR, 0xFFFFFFFF);

	// Регистрация интерфейса в ядре
    e1000_dev->nic = new_nic();
    memcpy(e1000_dev->nic->mac, e1000_dev->mac, MAC_DEFAULT_LEN);
    e1000_dev->nic->dev = dev; 
    e1000_dev->nic->flags = NIC_FLAG_UP | NIC_FLAG_BROADCAST | NIC_FLAG_MULTICAST;
    e1000_dev->nic->tx = e1000_tx;
    e1000_dev->nic->up = e1000_up;
    e1000_dev->nic->down = e1000_down;
    e1000_dev->nic->mtu = 1500; // уточнить
    register_nic(e1000_dev->nic, "eth");

    dev->dev_type = DEVICE_TYPE_NETWORK_ADAPTER;
    dev->dev_data = e1000_dev;
    dev->nic = e1000_dev->nic;

	return 0;
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

	printk("E1000: Getting MAC without EEPROM\n");

	if (e1000_read32(dev, 0x5400) == 0)
	{
		printk("E1000: No mac address\n");
		return 1;
	}

	for (int i = 0; i < 6; i++)
	{
		dev->mac[i] = (uint8_t) e1000_read32(dev, 0x5400 + i * 8);
	}

	return 1;
}

int e1000_init_rx(struct e1000* dev)
{
	// Выделить память под таблицу rx дескрипторов
	size_t rx_size = E1000_NUM_RX_DESCS * sizeof(struct e1000_rx_desc) + 16;
	size_t rx_npages = 0;
	dev->rx_table_phys = (uintptr_t) pmm_alloc(rx_size, &rx_npages);
	dev->rx_table = (struct e1000_rx_desc*) map_io_region(dev->rx_table_phys, rx_size);

	// Выделить память под массив виртуальных адресов
	dev->rx_buffers = (void**) kmalloc(E1000_NUM_RX_DESCS * sizeof(void*));

	for (int i = 0; i < E1000_NUM_RX_DESCS; i ++)
	{
		size_t rx_buf_pages = 0;
		void* rx_buffer_phys = pmm_alloc(E1000_TX_BUF_SZ + 16, &rx_buf_pages); // 8192 + 16

		struct e1000_rx_desc* desc = &dev->rx_table[i];
		desc->addr = (uint64_t) rx_buffer_phys;
		desc->status = 0;

		dev->rx_buffers[i] = (void*) map_io_region(rx_buffer_phys, rx_buf_pages * PAGE_SIZE); 
	}

	// Записать адрес
	e1000_write32(dev, E1000_REG_RDBAHI, dev->rx_table_phys >> 32);
	e1000_write32(dev, E1000_REG_RDBALO, dev->rx_table_phys & UINT32_MAX);
	// Записать количество
	e1000_write32(dev, E1000_REG_RDLEN, E1000_NUM_RX_DESCS * sizeof(struct e1000_rx_desc));
	// Записать позиции головы и хвоста
	e1000_write32(dev, E1000_REG_RDHEAD, 0);
	e1000_write32(dev, E1000_REG_RDTAIL, E1000_NUM_RX_DESCS - 1);
	// Включить
	e1000_write32(dev, E1000_REG_RCTL, 
		RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_8192);

	// Инициализиовать тасклет
	tasklet_init(&dev->rx_tasklet, e1000_rx, dev);

	return 0;
}

int e1000_init_tx(struct e1000* dev)
{
	// Выделить память под таблицу rx дескрипторов
	size_t tx_size = E1000_NUM_TX_DESCS * sizeof(struct e1000_tx_desc) + 16;
	size_t tx_npages = 0;
	dev->tx_table_phys = (uintptr_t) pmm_alloc(tx_size, &tx_npages);
	dev->tx_table = (struct e1000_tx_desc*) map_io_region(dev->tx_table_phys, tx_size);

	// Выделить память под массив виртуальных адресов
	dev->tx_buffers = (void**) kmalloc(E1000_NUM_TX_DESCS * sizeof(void*));

	for (int i = 0; i < E1000_NUM_TX_DESCS; i ++)
	{
		size_t tx_buf_pages = 0;
		void* tx_buffer_phys = pmm_alloc(E1000_TX_BUF_SZ + 16, &tx_buf_pages);

		struct e1000_tx_desc* desc = &dev->tx_table[i];
		desc->addr = (uint64_t) tx_buffer_phys;
		desc->cmd = 0;

		dev->tx_buffers[i] = (void*) map_io_region(tx_buffer_phys, tx_buf_pages * PAGE_SIZE); 
	}

	// Записать адрес
	e1000_write32(dev, E1000_REG_TDBAHI, dev->tx_table_phys >> 32);
	e1000_write32(dev, E1000_REG_TDBALO, dev->tx_table_phys & UINT32_MAX);
	// Записать количество
	e1000_write32(dev, E1000_REG_TDLEN, E1000_NUM_TX_DESCS * sizeof(struct e1000_tx_desc));
	// Позиции
	e1000_write32(dev, E1000_REG_TDHEAD, 0);
	e1000_write32(dev, E1000_REG_TDTAIL, 0);
	// Включение + выравнивание
	e1000_write32(dev, E1000_REG_TCTL, TCTL_EN | TCTL_PSP);
	// ???
	e1000_write32(dev, E1000_REG_TIPG, 0x0060200A);

	return 0;
}

void e1000_enable_link(struct e1000* dev)
{
	e1000_write32(dev, E1000_REG_CTRL, e1000_read32(dev, E1000_REG_CTRL) | E1000_CTRL_SLU);
}

int e1000_up(struct nic* nic)
{
	return 0;
}

int e1000_down(struct nic* nic)
{
	return 0;
}

int e1000_tx(struct nic* nic, const unsigned char* buffer, size_t size)
{
	struct e1000* dev = (struct e1000*) nic->dev->dev_data;
	uint32_t tx_current = e1000_read32(dev, E1000_REG_TDTAIL) % E1000_NUM_TX_DESCS;

	//printk("E1000: Tx, current %i\n", tx_current);

	char* tx_buffer = dev->tx_buffers[tx_current];
	memcpy(tx_buffer, buffer, size);

	struct e1000_tx_desc* desc = &dev->tx_table[tx_current];
	desc->length = size;
	desc->status = 0;
	desc->cmd = CMD_EOP | CMD_IFCS | CMD_RS;

	e1000_write32(dev, E1000_REG_TDTAIL, (tx_current + 1) % E1000_NUM_TX_DESCS);

	while (desc->status == 0)
		continue;

	// Увеличить счетчики статистики
	dev->dev->nic->stats.tx_packets++;
    dev->dev->nic->stats.tx_bytes += size;

	return 0;
}

void e1000_rx(struct e1000* dev)
{
	while (1)
	{
		uint32_t rx_current = (e1000_read32(dev, E1000_REG_RDTAIL) + 1) % E1000_NUM_RX_DESCS;

		struct e1000_rx_desc* desc = &dev->rx_table[rx_current];

		if (!(desc->status & 1))
			break;

		//printk("E1000: Rx, current %i\n", rx_current);

		if ((desc->errors & 0x97) != 0)
		{
			//printk("E1000: RX Error %i\n", desc->errors);
			dev->dev->nic->stats.rx_errors += 1;
			dev->dev->nic->stats.rx_dropped += 1;
		}

		size_t payload_len = desc->length;
		char* rx_buffer = dev->rx_buffers[rx_current];

		// Увеличить счетчики статистики
		dev->dev->nic->stats.rx_packets++;
		dev->dev->nic->stats.rx_bytes += payload_len;

		// Собрать и обработать пакет в ядре
		struct net_buffer* nb = new_net_buffer(rx_buffer, payload_len, dev->nic);
		net_buffer_acquire(nb);
		eth_handle_frame(nb);
		net_buffer_free(nb);

		desc->errors = 0;
		desc->status = 0;

		// Запись смещения
		e1000_write32(dev, E1000_REG_RDTAIL, rx_current);
	}
}

void e1000_irq_handler(void* regs, struct e1000* dev) 
{
	uint32_t status = e1000_read32(dev, E1000_REG_ICR);
	e1000_write32(dev, E1000_REG_ICR, status);

	//printk("E1000: IRQ Handled! %i\n", status);

	if (status & ICR_TXDW)
	{
		//printk("Transmitted\n");
	}

	if (status & ICR_LSC)
	{
	}
	else if (status & ICR_RX0)
	{
		dev->dev->nic->stats.rx_errors += 1;
		dev->dev->nic->stats.rx_overruns += 1;
	}
	else if (status & ICR_RXSEQ)
	{
		printk("E1000: RXSEQ\n");
	}
	else if (status & (ICR_RxQ0 | ICR_RXT0)) 
	{
		tasklet_schedule(&dev->rx_tasklet);
	}
	else
	{
		printk("E1000: IRQ status %i\n", status);
	}
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