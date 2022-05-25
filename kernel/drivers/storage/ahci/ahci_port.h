#ifndef _AHCI_PORT_H
#define _AHCI_PORT_H

#include "ahci_defines.h"
#include "sync/spinlock.h"

#define AHCI_PORT_SPEED_1_5_GBPS    1
#define AHCI_PORT_SPEED_3_0_GBPS    2
#define AHCI_PORT_SPEED_6_0_GBPS    3
#define AHCI_PORT_SPEED_UNRESTRICTED    4

typedef struct PACKED {
    uint32_t            index;
    HBA_PORT*           port_reg;
    ahci_device_type    device_type;
    int speed;

    spinlock_t          spinlock;

    HBA_COMMAND*        command_list;
    fis_t*              fis;
} ahci_port_t;

ahci_port_t* initialize_port(ahci_port_t* port, uint32_t index, HBA_PORT* port_desc);

void ahci_port_power_on(ahci_port_t* port);

void ahci_port_spin_up(ahci_port_t* port);

void ahci_port_activate_link(ahci_port_t* port);

void ahci_enable_fis_receive_enable(ahci_port_t* port);

void ahci_port_clear_error_register(ahci_port_t* port);

static inline void ahci_port_flush_posted_writes(ahci_port_t* port)
{
	volatile uint32_t dummy = port->port_reg->cmd;
	dummy = dummy;

}

#endif