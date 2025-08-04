#ifndef _RTL_8169_H
#define _RTL_8169_H

#include "kairax/types.h"

#define REALTEK_PCIID     0x10EC

#define RTL8169_CR          0x37
#define RTL8169_9346CR      0x50


#define RTL8169_CR_RST      0x10


#define RTL8169_9346_NORMAL 0
#define RTL8169_9346_CONFIG (0b11000000)


struct rtl8169 {
    struct device* dev;
    struct nic* nic;
    uint32_t io_addr;
    uint8_t mac[6];
};

int module_init(void);
void module_exit(void);

int rtl8169_device_probe(struct device *dev);

int rtl8169_reset(struct rtl8169* rtl_dev);
void rtl8169_read_mac(struct rtl8169* rtl_dev);
void rtl8169_lock_unlock_9346(struct rtl8169* rtl_dev, int unlock);

#endif