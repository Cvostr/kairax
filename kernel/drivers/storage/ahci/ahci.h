#ifndef _AHCI_H
#define _AHCI_H

#include "bus/pci/pci.h"
#include "ahci_defines.h"

#define AHCI_CAPABILITY_64BIT       (uint32_t)(1 << 31)
#define AHCI_CAPABILITY_NCQ         (1 << 30)
#define AHCI_CAPABILITY_SNTF        (1 << 29)       //SNotification Registe
#define AHCI_CAPABILITY_MPSW         (1 << 28)       //Mechanical Presence Switch
#define AHCI_CAPABILITY_SSS         (1 << 27)       //Staggered Spin-up
#define AHCI_CAPABILITY_ALPOWERM         (1 << 26)       //Aggressive Link Power Management
#define AHCI_CAPABILITY_AL          (1 << 25)       //Activity LED
#define AHCI_CAPABILITY_CLO         (1 << 24)       //Command List Override
#define AHCI_CAPABILITY_AM          (1 << 18)       //AHCI Mode Only
#define AHCI_CAPABILITY_PMUL          (1 << 17)       //Port multiplier


#define AHCI_CAPABILITY_EX_DEVSLEEP         (1 << 3)
#define AHCI_CAPABILITY_EX_NVMHCI           (1 << 1)
#define AHCI_CAPABILITY_EX_BHOFF            (1)

#include "ahci_port.h"

typedef struct PACKED {
    pci_device_desc*    pci_device;
    HBA_MEMORY*         hba_mem;

    ahci_port_t*        ports[32];
} ahci_controller_t;

int ahci_controller_reset(ahci_controller_t* controller);

void ahci_controller_enable_interrupts_ghc(ahci_controller_t* controller);

void ahci_controller_probe_ports(ahci_controller_t* controller);

void ahci_init();

#endif
