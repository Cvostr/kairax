#ifndef _RTL8139_H_
#define _RTL8139_H_

#include "kairax/types.h"

#define RX_BUFFER_SIZE (8192 + 1500 + 16)
#define TX_BUFFERS      4

#define RTL1839_TOK     0x04
#define RTL1839_ROK     0x01

#define RTL8139_RBSTART 0x30
#define RTL8139_CMD     0x37
#define RTL8139_CAPR    0x38
#define RTL8139_ISR     0x3E

#define RTL8139_RX_BUFFER_EMPTY 0x1

#define RTL8139_OK          0x1
#define RTL8139_BAD_ALIGN   0x2
#define RTL8139_CRCERR      0x4
#define RTL8139_LONG        0x8
#define RTL8139_RUNT        0x10
#define RTL8139_ISE         0x20

struct rtl8139 {
    struct device* dev;
    uint32_t io_addr;
    uint8_t mac[6];
    
    char* recv_buffer;
    char* tx_buffers[TX_BUFFERS];

    uint16_t rx_pos;
    uint8_t  tx_pos;
};

void rtl8139_rx(struct rtl8139* rtl_dev);
int rtl8139_tx(struct device* dev, const unsigned char* buffer, size_t size);

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

#endif