#ifndef _E1000_H
#define _E1000_H

#include "kairax/types.h"

#define INTEL_PCIID     0x8086

#define DEVID_EMULATED  0x100E
#define DEVID_I217      0x153A

#define E1000_REG_STATUS    0x0008
#define E1000_REG_EEPROM    0x0014

struct e1000 {
    struct device* dev;
    struct nic* nic;
    struct pci_device_bar* BAR;
    uintptr_t mmio;
    uint8_t mac[6];  

    int has_eeprom;
};

int module_init(void);
void module_exit(void);

int e1000_device_probe(struct device *dev);


void e1000_detect_eeprom(struct e1000* dev);
uint16_t e1000_eeprom_read(struct e1000* dev, uint8_t addr);
int e1000_get_mac(struct e1000* dev);
int e1000_init_rx(struct e1000* dev);
int e1000_init_tx(struct e1000* dev);

// Операции с регистрами
void e1000_write32(struct e1000* dev, off_t offset, uint32_t value);
uint32_t e1000_read32(struct e1000* dev, off_t offset);


static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile("outl %%eax, %%dx" :: "d" (port), "a"(val));
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