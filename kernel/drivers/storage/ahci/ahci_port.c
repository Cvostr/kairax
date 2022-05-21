#include "ahci_port.h"
#include "memory/pmm.h"

static int get_device_type(HBA_PORT *port)
{
	uint32_t ssts = port->ssts;
 
	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;

	if (det != HBA_PORT_DET_PRESENT)	// Check drive status
		return AHCI_DEV_NULL;
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEV_NULL;

	switch (port->sig)
	{
	case SATA_SIG_ATAPI:
		return AHCI_DEV_SATAPI;
	case SATA_SIG_SEMB:
		return AHCI_DEV_SEMB;
	case SATA_SIG_PM:
		return AHCI_DEV_PM;
	default:
		return AHCI_DEV_SATA;
	}
}

ahci_port_t* initialize_port(uint32_t index, HBA_PORT* port_desc){
    ahci_device_type device_type = get_device_type(port_desc);

    ahci_port_t* result = alloc_page();
    memset(result, 0, sizeof(ahci_port_t));

    result->port_reg = port_desc;
    result->index = index;
    result->device_type = device_type;

    result->command_list = (HBA_COMMAND*)alloc_page();
    result->command_table = (HBA_COMMAND_TABLE*)alloc_page();
    result->fis = (fis_t*)alloc_page();
    result->prd = (HBA_PRDT_ENTRY*)alloc_page();

    return result;
}