#ifndef _E1000_H
#define _E1000_H

#include "kairax/types.h"
#include "proc/tasklet.h"

#define INTEL_PCIID     0x8086

#define DEVID_EMULATED  0x100E
#define DEVID_I217      0x153A

#define E1000_REG_CTRL	    0x0000
#define E1000_REG_STATUS    0x0008
#define E1000_REG_EEPROM    0x0014
#define E1000_REG_ICR		0x00C0
#define E1000_REG_ITR		0x00C4
#define E1000_REG_IMASK		0x00D0
#define E1000_REG_RCTL	    0x0100
#define E1000_REG_TCTL      0x0400
#define E1000_REG_TIPG	    0x0410
#define E1000_REG_RDBALO	0x2800
#define E1000_REG_RDBAHI	0x2804
#define E1000_REG_RDLEN	    0x2808
#define E1000_REG_RDHEAD	0x2810
#define E1000_REG_RDTAIL    0x2818
#define	E1000_REG_TDBALO	0x3800
#define E1000_REG_TDBAHI	0x3804
#define E1000_REG_TDLEN	    0x3808
#define E1000_REG_TDHEAD    0x3810
#define E1000_REG_TDTAIL	0x3818

// ------
#define E1000_CTRL_SLU		(1 << 6)

// -----------
#define RCTL_EN		        (1 << 1)        // Receiver Enable
#define RCTL_SBP	        (1 << 2)       // Store Bad Packets
#define RCTL_UPE	        (1 << 3)     // Unicast Promiscuous Enabled
#define RCTL_MPE            (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LBM_NONE       (0 << 6)    // No Loopback
#define RTCL_RDMTS_HALF     (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RCTL_BAM            (1 << 15)   // Broadcast Accept Mode
#define RCTL_CFIEN          (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI            (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF            (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF           (1 << 23)   // Pass MAC Control Frames
#define RCTL_SECRC          (1 << 26)   // Strip Ethernet CRC
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))

// -----------
#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision

// -----
#define ICR_LSC             (1 << 2)
#define ICR_RXSEQ           (1 << 3)
#define ICR_RXDMT0          (1 << 4)
#define ICR_RX0             (1 << 6)
#define ICR_RXT0            (1 << 7)
#define ICR_RxQ0	        (1 << 20)

// ----
#define	CMD_EOP		(1 << 0)    // End of Packet
#define	CMD_IFCS	(1 << 1)    // Insert FCS
#define CMD_IC		(1 << 2)    // Insert Checksum
#define CMD_RS		(1 << 3)   // Report Status
#define CMD_DEXT	(1 << 5)   // Report Packet Sent
#define	CMD_VLE		(1 << 6)   // VLAN Packet Enable
#define	CMD_IDE		(1 << 7)   // Interrupt Delay Enable

struct e1000_rx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint16_t checksum;
    volatile uint8_t status;
    volatile uint8_t errors;
    volatile uint16_t special;
} PACKED;

struct e1000_tx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint8_t cso;
    volatile uint8_t cmd;
    volatile uint8_t status;
    volatile uint8_t css;
    volatile uint16_t special;
} PACKED;

struct e1000 {
    struct device* dev;
    struct nic* nic;
    struct pci_device_bar* BAR;
    uintptr_t mmio;
    uint8_t mac[6];  

    int has_eeprom;

    uintptr_t rx_table_phys;
    struct e1000_rx_desc* rx_table;
    void** rx_buffers;

    uintptr_t tx_table_phys;
    struct e1000_tx_desc* tx_table;
    void** tx_buffers;

    struct tasklet rx_tasklet;
};

int module_init(void);
void module_exit(void);

int e1000_device_probe(struct device *dev);

// Внутренние операции
void e1000_detect_eeprom(struct e1000* dev);
uint16_t e1000_eeprom_read(struct e1000* dev, uint8_t addr);
int e1000_get_mac(struct e1000* dev);
int e1000_init_rx(struct e1000* dev);
int e1000_init_tx(struct e1000* dev);
void e1000_enable_link(struct e1000* dev);
void e1000_link_up(struct e1000* dev);
void e1000_irq_handler(void* regs, struct e1000* dev);
void e1000_rx(struct e1000* dev);

// Операции с регистрами
void e1000_write32(struct e1000* dev, off_t offset, uint32_t value);
uint32_t e1000_read32(struct e1000* dev, off_t offset);

// Операции для nic
int e1000_tx(struct nic* nic, const unsigned char* buffer, size_t size);
int e1000_up(struct nic* nic);
int e1000_down(struct nic* nic);

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