#ifndef _AHCI_DEFINES_H
#define _AHCI_DEFINES_H

#include "stdint.h"

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

typedef uint32_t ahci_device_type;

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	    0x96690101	// Port multiplier

#define AHCI_DEV_BUSY 0x80
#define AHCI_DEV_DRQ 0x08

#define HBA_PORT_SPD_MASK		0x000000f0	// Interface speed

#define COMMAND_LIST_ENTRY_COUNT 32
#define PRD_TABLE_ENTRY_COUNT COMMAND_LIST_ENTRY_COUNT * 8 //8 per each cmdslot

#define HBA_PxIS_TFE            (1 << 30)

typedef enum
{
	FIS_TYPE_REG_HOST_TO_DEV	= 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_DEV_TO_HOST	= 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACTIVATE	    = 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP	        = 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA		        = 0x46,	// Data FIS - bidirectional
	FIS_TYPE_BIST		        = 0x58,	// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP	        = 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS	        = 0xA1,	// Set device bits FIS - device to host
} FIS_TYPE;

typedef enum
{
    FIS_CMD_NONE                = 0,
    FIS_CMD_PACKET              = 0xa0,
    FIS_CMD_DEVICE_RESET        = 0x08,
    FIS_CMD_SERVICE             = 0xa2,
    FIS_CMD_GET_STATUS          = 0xda,

    FIS_CMD_FLUSH               = 0xe7,
    FIS_CMD_FLUSH_EXT           = 0xea,

    FIS_CMD_DATA_MANAGEMENT     = 0x06,
    FIS_CMD_EJECT               = 0xed,

    FIS_CMD_IDENTIFY_DEVICE     = 0xec,
    FIS_CMD_IDENTIFY_PACKET_DEVICE = 0xa1,

    FIS_CMD_READ_DMA            = 0xc8,
    FIS_CMD_READ_DMA_EXT        = 0x25,
    FIS_CMD_WRITE_DMA           = 0xca,
    FIS_CMD_WRITE_DMA_EXT       = 0x35,

    FIS_CMD_READ_DMA_QUEUEUD    = 0xc7,
    FIS_CMD_READ_DMA_QUEUED_EXT = 0x26,
    FIS_CMD_WRITE_DMA_QUEUED    = 0xcc,
    FIS_CMD_WRITE_DMA_QUEUED_EXT = 0x36,

    FIS_CMD_READ_MULTIPLE       = 0xc4,
    FIS_CMD_READ_MULTIPLE_EXT   = 0x29,
    FIS_CMD_WRITE_MULTIPLE      = 0xc5,
    FIS_CMD_WRITE_MULTPLE_EXT   = 0x39,

    FIS_CMD_READ_SECTORS        = 0x20,
    FIS_CMD_READ_SECTORS_EXT    = 0x24,
    FIS_CMD_WRITE_SECTORS       = 0x30,
    FIS_CMD_WRITE_SECTORS_EXT   = 0x34,
} FIS_COMMAND;

typedef volatile struct PACKED
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;

    uint8_t reserved : 3;
    uint8_t cmd_ctrl : 1; //Если 1 - то команда, 0 - управление
    uint8_t command;
    uint8_t feature_l;
    uint8_t lba[3];
    uint8_t device;
    uint8_t lba2[3];
    uint8_t feature_h;

    uint8_t count_l;
    uint8_t count_h;
    uint8_t isochronous_command;
    uint8_t control;
    uint8_t reserved2[4];
} FIS_HOST_TO_DEV;


typedef volatile struct PACKED
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;

    uint8_t reserved : 2;
    uint8_t interrupt : 1;
    uint8_t reserved1 : 1;
    uint8_t status;
    uint8_t error;
    uint8_t lba[3];
    uint8_t device;
    uint8_t lba2[3];
    uint8_t reserved2;
    uint8_t countl;
    uint8_t counth;
    uint8_t reserved3[5];
} FIS_DEV_TO_HOST;

typedef volatile struct __attribute__((packed))
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;

    uint8_t reserved : 4;
    uint8_t reserved1[2];
    uint32_t data[];
} FIS_DATA;

typedef volatile struct PACKED
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;

    uint8_t reserved : 1;
    uint8_t direction : 1; // 1: dev->host
    uint8_t interrupt : 1;
    uint8_t auto_activate : 1;
    uint8_t reserved1[2];
    uint64_t dma_buffer_id;
    uint32_t reserved2;
    uint32_t dma_buffer_offset;
    uint32_t transfer_count;
    uint32_t reserved3;
} FIS_DMA_SETUP;

typedef volatile struct __attribute__((packed))
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;

    uint8_t reserved : 1;
    uint8_t direction : 1; // 1: dev -> host
    uint8_t interrupt : 1;
    uint8_t reserved1 : 1;
    uint8_t status;
    uint8_t error;
    uint8_t lba[3];
    uint8_t device;
    uint8_t lba2[3];
    uint8_t reserved2;
    uint8_t count_l;
    uint8_t count_h;
    uint8_t reserved3;
    uint8_t e_status;
    uint16_t transfer_count;
    uint8_t reserved4[2];

} FIS_PIO_SETUP;

typedef volatile struct __attribute__((packed))
{
    FIS_DMA_SETUP dma_setup_fis;
    uint8_t pad[4];

    FIS_PIO_SETUP pio_setup_fis;
    uint8_t pad1[12];

    FIS_DEV_TO_HOST d2h_fis;
    uint8_t pad2[4];

    uint8_t sdbfis[2];
    uint8_t pad3[64];
    uint8_t reserved[96];
} fis_t;

//HBA структуры

typedef volatile struct PACKED
{
	uint32_t clb;		// 0x00, command list base address, 1K-byte aligned
	uint32_t clbu;		// 0x04, command list base address upper 32 bits
	uint32_t fb;		// 0x08, FIS base address, 256-byte aligned
	uint32_t fbu;		// 0x0C, FIS base address upper 32 bits
	uint32_t is;		// 0x10, interrupt status
	uint32_t ie;		// 0x14, interrupt enable
	uint32_t cmd;		// 0x18, command and status
	uint32_t rsv0;		// 0x1C, Reserved
	uint32_t tfd;		// 0x20, task file data
	uint32_t sig;		// 0x24, signature
	uint32_t ssts;		// 0x28, SATA status (SCR0:SStatus)
	uint32_t sctl;		// 0x2C, SATA control (SCR2:SControl)
	uint32_t serr;		// 0x30, SATA error (SCR1:SError)
	uint32_t sact;		// 0x34, SATA active (SCR3:SActive)
	uint32_t ci;		// 0x38, command issue
	uint32_t sntf;		// 0x3C, SATA notification (SCR4:SNotification)
	uint32_t fbs;		// 0x40, FIS-based switch control
    uint32_t devslp;  // 0x44, Device sleep
	uint32_t rsv1[10];	// 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4];	// 0x70 ~ 0x7F, vendor specific
} HBA_PORT;

typedef volatile struct PACKED
{
	// 0x00 - 0x2B, Generic Host Control
	uint32_t cap;		    // 0x00, Host capability
	uint32_t ghc;		    // 0x04, Global host control
	uint32_t is;		    // 0x08, Interrupt status
	uint32_t pi;		    // 0x0C, Port implemented
	uint32_t version;		    // 0x10, Version
	uint32_t ccc_ctl;	    // 0x14, Command completion coalescing control
	uint32_t ccc_pts;	    // 0x18, Command completion coalescing ports
	uint32_t em_loc;		// 0x1C, Enclosure management location
	uint32_t em_ctl;		// 0x20, Enclosure management control
	uint32_t cap2;		// 0x24, Host capabilities extended
	uint32_t bohc;		// 0x28, BIOS/OS handoff control and status
	uint8_t  reserved[52];
    uint8_t   nvmhci[64];
	uint8_t  vendor[96];
	HBA_PORT	ports[32];	// 1 ~ 32
} HBA_MEMORY;

typedef volatile struct PACKED
{
    uint8_t cmd_fis_len : 5; // multiplied by 4 byte
    uint8_t atapi : 1;
    uint8_t write : 1;
    uint8_t prefetchable : 1;

    uint8_t reset : 1;
    uint8_t bist : 1;
    uint8_t clear_on_ok : 1;
    uint8_t reserved : 1;
    uint8_t port_multiplier : 4;
    uint16_t prdtl;
    volatile uint32_t prdbc;

    uint32_t ctdba_low;
    uint32_t ctdba_up;

    uint32_t reserved1[4];
} HBA_COMMAND;

typedef struct PACKED
{
	uint32_t dba;		// Data base address
	uint32_t dbau;		// Data base address upper 32 bits
	uint32_t rsv0;		// Reserved
	uint32_t dbc:22;		// Byte count, 4M max
	uint32_t rsv1:9;		// Reserved
	uint32_t i:1;		// Interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct PACKED
{
	uint8_t  cfis[64];	// Command FIS
	uint8_t  acmd[16];	// ATAPI command, 12 or 16 bytes
	uint8_t  reserved[48];	    
	HBA_PRDT_ENTRY	prdt_entry[8];	// Physical region descriptor table entries, 0 ~ 65535
} HBA_COMMAND_TABLE;

#endif