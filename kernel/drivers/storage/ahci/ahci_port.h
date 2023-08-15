#ifndef _AHCI_PORT_H
#define _AHCI_PORT_H

#include "ahci_defines.h"
#include "sync/spinlock.h"

#define AHCI_PORT_SPEED_1_5_GBPS    1
#define AHCI_PORT_SPEED_3_0_GBPS    2
#define AHCI_PORT_SPEED_6_0_GBPS    3
#define AHCI_PORT_SPEED_UNRESTRICTED    4

#define HBA_PxCMD_POWER         (1UL << 2)
#define HBA_PxCMD_START         0x0001
#define HBA_PxCMD_SPINUP        (1 << 1)
#define HBA_PxCMD_FRE           0x0010
#define HBA_PxCMD_CR            0x8000
#define HBA_PxCMD_FR            0x4000

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

typedef struct PACKED {
    uint32_t            implemented;
    uint32_t            index;
    HBA_PORT*           port_reg;
    ahci_device_type    device_type;
    int speed;

    spinlock_t          spinlock;

    HBA_COMMAND*        command_list;
    fis_t*              fis;
} ahci_port_t;

ahci_port_t* initialize_port(ahci_port_t* port, uint32_t index, HBA_PORT* port_desc);

void ahci_port_init2(ahci_port_t* port);

int ahci_port_reset(ahci_port_t* port);

int ahci_port_disable(ahci_port_t* port);

int ahci_port_enable(ahci_port_t* port);

uint32_t ahci_port_get_free_cmdslot(ahci_port_t* port);

int ahci_port_read_lba48(ahci_port_t *port, uint64_t start, uint32_t count, uint16_t *buf);

int ahci_port_read_lba(ahci_port_t *port, uint64_t start, uint64_t count, uint16_t *buf);

int ahci_port_write_lba48(ahci_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf);

int ahci_port_write_lba(ahci_port_t *port, uint64_t start, uint64_t count, uint16_t *buf);

int ahci_port_interrupt(ahci_port_t *port);

// Получить информацию об устройстве. buffer - должен быть физическим
int ahci_port_identity(ahci_port_t *port, char* buffer);

void ahci_port_power_on(ahci_port_t* port);

void ahci_port_spin_up(ahci_port_t* port);

void ahci_port_activate_link(ahci_port_t* port);

void ahci_port_fis_receive_enable(ahci_port_t* port);

void ahci_port_clear_error_register(ahci_port_t* port);

void ahci_port_stop_commands(ahci_port_t* port);

void ahci_port_start_commands(ahci_port_t* port);

static inline void ahci_port_flush_posted_writes(ahci_port_t* port)
{
	volatile uint32_t dummy = port->port_reg->cmd;
	dummy = dummy;
}

uint32_t parse_identity_buffer(char* buffer,
                               uint16_t* device_type,
                               uint16_t* capabilities,
                               uint32_t* cmd_sets,
                               uint32_t* size,
                               char* model,
                               char* serial);

#endif