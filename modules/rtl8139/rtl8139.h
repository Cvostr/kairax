#ifndef _RTL8139_H_
#define _RTL8139_H_

#include "kairax/types.h"

#define RX_BUFFER_SIZE (8192 + 1500 + 16)
#define TX_BUFFERS      4

#define RTL1839_RXOK        0x01
#define RTL8139_RXERR       0x02
#define RTL1839_TXOK        0x04
#define RTL8139_TXERR       0x08

#define RTL8139_TXSTAT_HOST_OWNS    0x00002000
#define RTL8139_TXSTAT_UNDERRUN     0x00004000
#define RTL8139_TXSTAT_OK           0x00008000
#define RTL8139_TXSTAT_OUTOFWINDOW  0x20000000
#define RTL8139_TXSTAT_ABORTED      0x40000000
#define RTL8139_TXSTAT_CARRIER_LOST 0x80000000

#define RTL8139_RBSTART 0x30
#define RTL8139_CMD     0x37
#define RTL8139_CAPR    0x38
#define RTL8139_INTR    0x3C
#define RTL8139_ISR     0x3E
#define RTL8139_RXCONF  0x44

#define RTL8139_CMD_TX_ENB  0x4
#define RTL8139_CMD_RX_ENB  0x8
#define RTL8139_CMD_RESET   0x10

#define RTL8139_RX_BUFFER_EMPTY 0x1

#define RTL8139_OK          0x1
#define RTL8139_BAD_ALIGN   0x2
#define RTL8139_CRCERR      0x4
#define RTL8139_LONG        0x8
#define RTL8139_RUNT        0x10
#define RTL8139_ISE         0x20

struct rtl8139 {
    struct device* dev;
    struct nic* nic;
    uint32_t io_addr;
    uint8_t mac[6];
    
    char* recv_buffer;
    char* tx_buffers[TX_BUFFERS];

    uint16_t rx_pos;
    uint32_t  tx_pos;

    // Позиция transmission в прерывании
    uint32_t int_tx_pos;   
};

void rtl8139_rx(struct rtl8139* rtl_dev);
int rtl8139_tx(struct nic* nic, const unsigned char* buffer, size_t size);

int rtl8139_up(struct nic* nic);
int rtl8139_down(struct nic* nic);

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline void outw(uint16_t port, uint16_t val)
{
    asm volatile ( "outw %%ax, %%dx" : : "a"(val), "d"(port) );
}

static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile("outl %%eax, %%dx" :: "d" (port), "a"(val));
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm ( "inw %%dx, %%ax"
                   : "=a"(ret)
                   : "d"(port) );
    return ret;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ( "inl %%dx, %%eax"
                   : "=a"(ret)
                   : "d"(port) );
    return ret;
}

#endif