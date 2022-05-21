#ifndef _AHCI_PORT_H
#define _AHCI_PORT_H

#include "ahci_defines.h"
#include "sync/spinlock.h"

typedef struct PACKED {
    uint32_t            index;
    HBA_PORT*           port_reg;
    ahci_device_type    device_type;

    spinlock_t          spinlock;

    HBA_COMMAND*        command_list;
    HBA_COMMAND_TABLE*  command_table;
    fis_t*              fis;
    HBA_PRDT_ENTRY*     prd;
} ahci_port_t;

ahci_port_t* initialize_port(uint32_t index, HBA_PORT* port_desc);

#endif