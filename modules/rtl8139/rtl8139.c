#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "kairax/string.h"
#include "rtl8139.h"

//#define LOG_ENABLED
//#define ON_RECEIVE_LOG_ENABLED
//#define IRQ_LOG_ENABLED
#define FAILURE_LOG_ENABLED

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

    int rx_pages = 3;

    // Выделение 3х страниц для буфера приема
    rtl_dev->recv_buffer = (char*) pmm_alloc_pages(rx_pages);
#ifdef LOG_ENABLED
    printk("Receive buffer address: %i\n", rtl_dev->recv_buffer);
#endif

    if (rtl_dev->recv_buffer == 0) {
        return -1;
    }

    // Сетевая карта работает только с 32х битным адресом
    // Если в этой границе свободных страниц нет, то нам не повезло
    if (rtl_dev->recv_buffer >= 0xFFFFFFFF) {
        pmm_free_pages(rtl_dev->recv_buffer, rx_pages);
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
    outb(rtl_dev->io_addr + RTL8139_CMD, RTL8139_CMD_RESET);
    while( (inb(rtl_dev->io_addr + RTL8139_CMD) & RTL8139_CMD_RESET) != 0);

    for (int i = 0; i < MAC_DEFAULT_LEN; i ++) {
        rtl_dev->mac[i] = inb(rtl_dev->io_addr + i);
    }    

    // Установка адреса receive buffer
    outl(rtl_dev->io_addr + RTL8139_RBSTART, (uint32_t) rtl_dev->recv_buffer);
    // Сохранение виртуального адреса receive buffer
    rtl_dev->recv_buffer = vmm_get_virtual_address(rtl_dev->recv_buffer);
    // Зануляем
    memset(rtl_dev->recv_buffer, 0, PAGE_SIZE * rx_pages);

    // ISR + IMR
    outw(rtl_dev->io_addr + RTL8139_INTR, RTL1839_RXOK | RTL1839_TXOK);

    // AB + AM + APM + AAP + WRAP
    outl(rtl_dev->io_addr + RTL8139_RXCONF, 0xf | RTL8139_RX_WRAP);

    // Включение rx + tx
    outb(rtl_dev->io_addr + RTL8139_CMD, RTL8139_CMD_TX_ENB | RTL8139_CMD_RX_ENB);

    int irq = pci_device_get_irq_line(dev->pci_info);
#ifdef LOG_ENABLED
    printk("IRQ %i\n", irq);
#endif
    register_irq_handler(irq, rtl8139_irq_handler, rtl_dev);

    rtl_dev->nic = new_nic();
    memcpy(rtl_dev->nic->mac, rtl_dev->mac, MAC_DEFAULT_LEN);
    rtl_dev->nic->dev = dev; 
    rtl_dev->nic->flags = NIC_FLAG_UP | NIC_FLAG_BROADCAST | NIC_FLAG_MULTICAST;
    rtl_dev->nic->tx = rtl8139_tx;
    rtl_dev->nic->up = rtl8139_up;
    rtl_dev->nic->down = rtl8139_down;
    rtl_dev->nic->mtu = 1500; // уточнить
    register_nic(rtl_dev->nic, "eth");

    dev->dev_type = DEVICE_TYPE_NETWORK_ADAPTER;
    dev->dev_data = rtl_dev;
    dev->nic = rtl_dev->nic;

    struct process* proc = create_new_process(NULL);
    struct thread* thr = create_kthread(proc, routine, rtl_dev);
    scheduler_add_thread(thr);

    return 0;
}

int rtl8139_up(struct nic* nic)
{
#ifdef LOG_ENABLED
    printk("RTL8139: up\n");
#endif
    // todo: implement
    struct rtl8139* rtl_dev = (struct rtl8139*) nic->dev->dev_data;
    outb(rtl_dev->io_addr + RTL8139_CMD, RTL8139_CMD_TX_ENB | RTL8139_CMD_RX_ENB);
    // ISR + IMR
    outw(rtl_dev->io_addr + RTL8139_INTR, RTL1839_RXOK | RTL1839_TXOK);
    return 0;
}

int rtl8139_down(struct nic* nic)
{
#ifdef LOG_ENABLED
    printk("RTL8139: down\n");
#endif
    // todo: implement
    struct rtl8139* rtl_dev = (struct rtl8139*) nic->dev->dev_data;
    outb(rtl_dev->io_addr + RTL8139_CMD, 0x0);
    // ISR + IMR
    outw(rtl_dev->io_addr + RTL8139_INTR, 0x0);
    return 0;
}

void rtl8139_rx(struct rtl8139* rtl_dev)
{
    char*   rx_buffer;
    uint16_t rx_status;
    uint16_t rx_len;
    uint16_t payload_len;
    uint16_t rx_pos = rtl_dev->rx_pos; 

    while ((inb(rtl_dev->io_addr + RTL8139_CMD) & RTL8139_RX_BUFFER_EMPTY) == 0) 
    {
        rx_buffer = rtl_dev->recv_buffer + rx_pos;
        rx_status = *((uint16_t*)rx_buffer);
        rx_len = *(((uint16_t*)rx_buffer) + 1);
        rx_buffer = (char*) (((uint16_t*)rx_buffer) + 2);

        if (rx_status & (RTL8139_ISE | RTL8139_CRCERR | RTL8139_RUNT | RTL8139_LONG | RTL8139_BAD_ALIGN)) 
        {
#ifdef FAILURE_LOG_ENABLED
            printk("Bad packed received (%i)!\n", rx_status);
#endif
            rtl_dev->dev->nic->stats.rx_errors++;

            if (rx_status & (RTL8139_ISE | RTL8139_BAD_ALIGN))
                rtl_dev->dev->nic->stats.rx_frame_errors++;

            if (rx_status & (RTL8139_LONG | RTL8139_RUNT))
                rtl_dev->dev->nic->stats.rx_overruns++;

            // Realtek рекомендует сбрасывать
            rtl_dev->rx_pos = 0;

            // Включение rx + tx
            outb(rtl_dev->io_addr + RTL8139_CMD, RTL8139_CMD_TX_ENB | RTL8139_CMD_RX_ENB);

        } else if (rx_status & RTL8139_OK) {
#ifdef ON_RECEIVE_LOG_ENABLED
            printk("8139: RX: P %i S %i L %i\n", rx_pos, rx_status, rx_len);
#endif

            payload_len = rx_len - 4; // Удаление длины CRC

            rtl_dev->dev->nic->stats.rx_packets++;
            rtl_dev->dev->nic->stats.rx_bytes += payload_len;

            struct net_buffer* nb = new_net_buffer(rx_buffer, payload_len, rtl_dev->nic);
            net_buffer_acquire(nb);
            eth_handle_frame(nb);
            net_buffer_free(nb);
        }

        rx_pos = (rx_pos + rx_len + 4 + 3) & (~3);

        // Так как включен WRAP можем просто вычесть 8192
        if (rx_pos > RX_BUFFER_SIZE) {
            rx_pos -= RX_BUFFER_SIZE;
        }

        outw(rtl_dev->io_addr + RTL8139_CAPR, rx_pos - 0x10); 
    }

    rtl_dev->rx_pos = rx_pos;
}

int rtl8139_tx(struct nic* nic, const unsigned char* buffer, size_t size)
{
    struct rtl8139* rtl_dev = (struct rtl8139*) nic->dev->dev_data;
    asm volatile("cli");

    uint32_t tx_pos = rtl_dev->tx_pos;

    int descriptor = tx_pos % TX_BUFFERS;

    unsigned char* tx_buffer_virt = vmm_get_virtual_address(rtl_dev->tx_buffers[descriptor]);
    memcpy(tx_buffer_virt, buffer, size);

    outl(rtl_dev->io_addr + TSAD[descriptor], rtl_dev->tx_buffers[descriptor]);
    outl(rtl_dev->io_addr + TSD[descriptor], size);

    rtl_dev->tx_pos = tx_pos + 1;

    asm volatile("sti");
    return 0;
}

void rtl8139_irq_handler(void* regs, struct rtl8139* rtl_dev) 
{
    uint16_t status = inw(rtl_dev->io_addr + RTL8139_ISR);
	outw(rtl_dev->io_addr + RTL8139_ISR, 0x05);
#ifdef IRQ_LOG_ENABLED
    printk("rtl8139_irq_handler() %i\n", status);
#endif

    if (status & (RTL1839_TXOK | RTL8139_TXERR)) {

        uint32_t int_tx_pos = rtl_dev->int_tx_pos;
        while (rtl_dev->tx_pos > int_tx_pos) 
        {
            int descriptor_num = int_tx_pos % TX_BUFFERS;
            int tx_status = inl(rtl_dev->io_addr + TSD[descriptor_num]);

            if (tx_status & (RTL8139_TXSTAT_OUTOFWINDOW | RTL8139_TXSTAT_ABORTED)) 
            {
#ifdef FAILURE_LOG_ENABLED
                printk("8139: Error sending packet, status: %i\n", tx_status);
#endif
                rtl_dev->dev->nic->stats.tx_errors++;
            } else {
                // Sent
#ifdef LOG_ENABLED
                printk("8139: Packed sent, status: %i\n", tx_status);
#endif

                rtl_dev->dev->nic->stats.tx_packets++;
                rtl_dev->dev->nic->stats.tx_bytes += tx_status & 0xFFF;
            }

            int_tx_pos++;
        } 

        rtl_dev->int_tx_pos = int_tx_pos;
	}
	if (status & (RTL1839_RXOK)) {
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

DEFINE_MODULE("rtl8139", init, exit)