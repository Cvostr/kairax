#ifndef _AHCI_DEFINES_H
#define _AHCI_DEFINES_H

#include "stdint.h"

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

typedef volatile struct __attribute__((packed))
{
    uint8 fis_type;
    uint8 port_multiplier : 4;

    uint8 reserved : 3;
    uint8 cmd_ctrl : 1; //Если 1 - то команда, 0 - управление
    uint8 command;
    uint8 feature_1;
    uint8 lba[3];
    uint8 device;
    uint8 lba2[3];
    uint8 feature_2;

    uint8 count_l;
    uint8 count_h;
    uint8 isochronous_command;
    uint8 control;
    uint8 reserved2[4];
} FIS_HOST_TO_DEV;


typedef volatile struct __attribute__((packed))
{
    uint8 fis_type;
    uint8 port_multiplier : 4;

    uint8 reserved : 2;
    uint8 interrupt : 1;
    uint8 reserved1 : 1;
    uint8 status;
    uint8 error;
    uint8 lba[3];
    uint8 device;
    uint8 lba2[3];
    uint8 reserved2;
    uint8 countl;
    uint8 counth;
    uint8 reserved3[5];
} FIS_DEV_TO_HOST;

typedef volatile struct __attribute__((packed))
{
    uint8 fis_type;
    uint8 port_multiplier : 4;

    uint8 reserved : 4;
    uint8 reserved1[2];
    uint32 data[];
} FIS_DATA;

typedef volatile struct __attribute__((packed))
{
    uint8 fis_type;
    uint8 port_multiplier : 4;

    uint8 reserved : 1;
    uint8 direction : 1; // 1: dev->host
    uint8 interrupt : 1;
    uint8 auto_activate : 1;
    uint8 reserved1[2];
    uint64 dma_buffer_id;
    uint32 reserved2;
    uint32 dma_buffer_offset;
    uint32 transfer_count;
    uint32 reserved3;
} FIS_DMA_SETUP;

typedef volatile struct __attribute__((packed))
{
    uint8 fis_type;
    uint8 port_multiplier : 4;

    uint8 reserved : 1;
    uint8 direction : 1; // 1: dev -> host
    uint8 interrupt : 1;
    uint8 reserved1 : 1;
    uint8 status;
    uint8 error;
    uint8 lba[3];
    uint8 device;
    uint8 lba2[3];
    uint8 reserved2;
    uint8 count_l;
    uint8 count_h;
    uint8 reserved3;
    uint8 e_status;
    uint16 transfer_count;
    uint8 reserved4[2];

} FIS_PIO_SETUP;

typedef volatile struct __attribute__((packed))
{
    FIS_DMA_SETUP dma_setup_fis;
    uint8 pad[4];

    FIS_PIO_SETUP pio_setup_fis;
    uint8 pad1[12];

    FIS_DEV_TO_HOST d2h_fis;
    uint8 pad2[4];

    uint8 sdbfis[2];
    uint8 pad3[64];
    uint8 reserved[96];
} Fis;

//HBA структуры

typedef volatile struct __attribute__((packed))
{
	uint32 clb;		// 0x00, command list base address, 1K-byte aligned
	uint32 clbu;		// 0x04, command list base address upper 32 bits
	uint32 fb;		// 0x08, FIS base address, 256-byte aligned
	uint32 fbu;		// 0x0C, FIS base address upper 32 bits
	uint32 is;		// 0x10, interrupt status
	uint32 ie;		// 0x14, interrupt enable
	uint32 cmd;		// 0x18, command and status
	uint32 rsv0;		// 0x1C, Reserved
	uint32 tfd;		// 0x20, task file data
	uint32 sig;		// 0x24, signature
	uint32 ssts;		// 0x28, SATA status (SCR0:SStatus)
	uint32 sctl;		// 0x2C, SATA control (SCR2:SControl)
	uint32 serr;		// 0x30, SATA error (SCR1:SError)
	uint32 sact;		// 0x34, SATA active (SCR3:SActive)
	uint32 ci;		// 0x38, command issue
	uint32 sntf;		// 0x3C, SATA notification (SCR4:SNotification)
	uint32 fbs;		// 0x40, FIS-based switch control
	uint32 rsv1[11];	// 0x44 ~ 0x6F, Reserved
	uint32 vendor[4];	// 0x70 ~ 0x7F, vendor specific
} HBA_PORT;

typedef volatile struct __attribute__((packed))
{
	// 0x00 - 0x2B, Generic Host Control
	uint32 cap;		// 0x00, Host capability
	uint32 ghc;		// 0x04, Global host control
	uint32 is;		// 0x08, Interrupt status
	uint32 pi;		// 0x0C, Port implemented
	uint32 vs;		// 0x10, Version
	uint32 ccc_ctl;	// 0x14, Command completion coalescing control
	uint32 ccc_pts;	// 0x18, Command completion coalescing ports
	uint32 em_loc;		// 0x1C, Enclosure management location
	uint32 em_ctl;		// 0x20, Enclosure management control
	uint32 cap2;		// 0x24, Host capabilities extended
	uint32 bohc;		// 0x28, BIOS/OS handoff control and status
	uint8  reserved[0xA0-0x2C];
	uint8  vendor[0x100-0xA0];
	HBA_PORT	ports[1];	// 1 ~ 32
} HBA_MEMORY;

typedef volatile struct __attribute__((packed))
{
    uint8 command_len : 5; // multiplied by 4 byte
    uint8 atapi : 1;
    uint8 write : 1;
    uint8 prefetchable : 1;

    uint8 reset : 1;
    uint8 bist : 1;
    uint8 clear_on_ok : 1;
    uint8 reserved : 1;
    uint8 port_multiplier : 4;
    uint16 physical_region_descriptor_count;
    uint32 physical_region_decriptor_byte_transferred;

    uint32 command_table_descriptor_low;
    uint32 command_table_descriptor_up;

    uint32 reserved1[4];
} HBA_COMMAND;

typedef struct __attribute__((packed)) 
{
	uint32 dba;		// Data base address
	uint32 dbau;		// Data base address upper 32 bits
	uint32 rsv0;		// Reserved
	uint32 dbc:22;		// Byte count, 4M max
	uint32 rsv1:9;		// Reserved
	uint32 i:1;		// Interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct __attribute__((packed))
{
	uint8  cfis[64];	// Command FIS
	uint8  acmd[16];	// ATAPI command, 12 or 16 bytes
	uint8  reserved[48];	    
	HBA_PRDT_ENTRY	prdt_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
} HBA_COMMAND_TABLE;

#endif