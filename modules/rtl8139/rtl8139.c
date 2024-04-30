#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "kairax/string.h"
#include "rtl8139.h"

void rtl8139_irq_handler(void* regs, struct rtl8139* rtl_dev);

uint8_t TSAD[4] = {0x20, 0x24, 0x28, 0x2C};
uint8_t TSD[4] = {0x10, 0x14, 0x18, 0x1C};

void routine(struct rtl8139* rtl_dev) {

    while(1) {
        rtl8139_rx(rtl_dev);
    }
}

int rtl8139_device_probe(struct device *dev) 
{
    struct rtl8139* rtl_dev = kmalloc(sizeof(struct rtl8139));
    memset(rtl_dev, 0, sizeof(struct rtl8139));
    rtl_dev->dev = dev;

    // Выделение 3х страниц для буфера приема
    rtl_dev->recv_buffer = (char*) pmm_alloc_pages(3);
    printk("Receive buffer address: %i\n", rtl_dev->recv_buffer);

    if (rtl_dev->recv_buffer == 0) {
        return -1;
    }

    // Сетевая карта работает только с 32х битным адресом
    // Если в этой границе свободных страниц нет, то нам не повезло
    if (rtl_dev->recv_buffer >= 0xFFFFFFFF) {
        return -1;
    }

    // Выделение страниц буферов отправки
    for (int i = 0; i < TX_BUFFERS; i ++) {
        rtl_dev->tx_buffers[i] = pmm_alloc_page();
        if (rtl_dev->tx_buffers[i] == 0) {
            return -1;
        }
    }

    pci_set_command_reg(dev->pci_info, pci_get_command_reg(dev->pci_info) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
    pci_device_set_enable_interrupts(dev->pci_info, 1);

    rtl_dev->io_addr = (uint32_t) dev->pci_info->BAR[0].address;

    // Включение
    outb(rtl_dev->io_addr + 0x52, 0x0);
    // Сброс
    outb(rtl_dev->io_addr + RTL8139_CMD, 0x10);
    while( (inb(rtl_dev->io_addr + RTL8139_CMD) & 0x10) != 0);

    for (int i = 0; i < 6; i ++) {
        rtl_dev->mac[i] = inb(rtl_dev->io_addr + i);
        //printk("%i ", rtl_dev->mac[i]);
    }    

    // Установка адреса receive buffer
    outl(rtl_dev->io_addr + RTL8139_RBSTART, (uint32_t) rtl_dev->recv_buffer);
    rtl_dev->recv_buffer = vmm_get_virtual_address(rtl_dev->recv_buffer);
    memset(rtl_dev->recv_buffer, 0, PAGE_SIZE * 3);

    // ISR + IMR
    outw(rtl_dev->io_addr + 0x3C, 0x0005);

    // AB + AM + APM + AAP + WRAP
    outl(rtl_dev->io_addr + RTL8139_RXCONF, 0xf | (1 << 7));

    // Включение rx + tx
    outb(rtl_dev->io_addr + RTL8139_CMD, 0x0C);

    int irq = dev->pci_info->interrupt_line;
    printk("IRQ %i\n", irq);
    register_irq_handler(irq, rtl8139_irq_handler, rtl_dev);

    struct net_device_info* net_dev = kmalloc(sizeof(struct net_device_info));
    memcpy(net_dev->mac, rtl_dev->mac, 6); 
    net_dev->tx = rtl8139_tx;
    net_dev->mtu = 1500; // уточнить
    net_dev->ipv4_addr = 15ULL << 24 | 2ULL << 16 | 10; // 10.0.2.15

    dev->dev_type = DEVICE_TYPE_NETWORK_ADAPTER;
    dev->dev_data = rtl_dev;
    dev->net_info = net_dev;

    struct process* proc = create_new_process(NULL);
    struct thread* thr = create_kthread(proc, routine, rtl_dev);
    scheduler_add_thread(thr);

    return 0;
}

void rtl8139_rx(struct rtl8139* rtl_dev)
{
    uint32_t ring_offset;
    char*   rx_buffer;
    uint16_t rx_status;
    uint16_t rx_len;
    uint16_t rx_pos = rtl_dev->rx_pos; 

    while ((inb(rtl_dev->io_addr + RTL8139_CMD) & RTL8139_RX_BUFFER_EMPTY) == 0) 
    {
        ring_offset = rx_pos % RX_BUFFER_SIZE;
        rx_buffer = rtl_dev->recv_buffer + ring_offset;
        rx_status = *((uint16_t*)rx_buffer);
        rx_len = *(((uint16_t*)rx_buffer) + 1);
        rx_buffer = (char*) (((uint16_t*)rx_buffer) + 2);

        if (rx_status & (RTL8139_ISE | RTL8139_CRCERR | RTL8139_RUNT | RTL8139_LONG | RTL8139_BAD_ALIGN)) {
            printk("Bad packed received!\n");
        } else if (rx_status & RTL8139_OK) {
            printk("Pos %i Status %i, Len %i\n", rx_pos, rx_status, rx_len);

            eth_handle_frame(rtl_dev->dev, rx_buffer, rx_len);
        }

        rx_pos = (rx_pos + rx_len + 4 + 3) & (~3);
        outw(rtl_dev->io_addr + RTL8139_CAPR, rx_pos - 0x10); 
    }

    rtl_dev->rx_pos = rx_pos;
}

int rtl8139_tx(struct device* dev, const unsigned char* buffer, size_t size)
{
    struct rtl8139* rtl_dev = (struct rtl8139*) dev->dev_data;
    asm volatile("cli");

    uint32_t tx_pos = rtl_dev->tx_pos;
    unsigned char* tx_buffer_virt = vmm_get_virtual_address(rtl_dev->tx_buffers[tx_pos]);
    memcpy(tx_buffer_virt, buffer, size);

    outl(rtl_dev->io_addr + TSAD[tx_pos], rtl_dev->tx_buffers[tx_pos]);
    outl(rtl_dev->io_addr + TSD[tx_pos], size);

    if ((++tx_pos) >= TX_BUFFERS) {
        tx_pos = 0;
    }
    rtl_dev->tx_pos = tx_pos;

    asm volatile("sti");
    return 0;
}

void rtl8139_irq_handler(void* regs, struct rtl8139* rtl_dev) 
{
    uint16_t status = inw(rtl_dev->io_addr + RTL8139_ISR);
	outw(rtl_dev->io_addr + RTL8139_ISR, 0x05);
    //printk("rtl8139_irq_handler() %i\n", status);

    if(status & RTL1839_TOK) {
		// Sent
        printk("Packed sent\n");
	}
	if (status & RTL1839_ROK) {
        //rtl8139_rx(rtl_dev);
	}
}

struct device_driver_ops rtl8139_ops = {
    .probe = rtl8139_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_id rtl8139_pci_id[] = {
	{PCI_DEVICE(0x10EC, 0x8139)},
	{0,}
};

struct pci_device_driver rtl8139_driver = {
	.dev_name = "Realtek RTL8139 Network card",
	.pci_device_ids = rtl8139_pci_id,
	.ops = &rtl8139_ops
};

int init(void)
{
    register_pci_device_driver(&rtl8139_driver);
	
	return 0;
}

void exit(void)
{

}

DEFINE_MODULE("hda", init, exit)