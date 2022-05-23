#include "ahci_port.h"
#include "memory/pmm.h"
#include "string.h"

#define LO32(val) ((uint32_t)(val))
#define HI32(val) ((uint32_t)(((uint64_t)(val)) >> 32))

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
	if(device_type == 0)
		return NULL;
		
    ahci_port_t* result = (ahci_port_t*)alloc_page();
    memset(result, 0, sizeof(ahci_port_t));

    result->port_reg = port_desc;
    result->index = index;
    result->device_type = device_type;

    size_t cmdlist_size = sizeof(HBA_COMMAND) * COMMAND_LIST_ENTRY_COUNT;
    size_t prd_size = sizeof(HBA_PRDT_ENTRY) * PRD_TABLE_ENTRY_COUNT;

    printf("%i %i", cmdlist_size, prd_size);

    HBA_COMMAND* cmdlist = (HBA_COMMAND*)alloc_page();
    HBA_COMMAND_TABLE* cmdtable = (HBA_COMMAND_TABLE*)alloc_page();
    fis_t* fis = (fis_t*)alloc_page();
    HBA_PRDT_ENTRY* prdt = (HBA_PRDT_ENTRY*)alloc_page();

    result->command_list = cmdlist;
    result->command_table = cmdtable;
    result->fis = fis;
    result->prd = prdt;

    port_desc->clb  = LO32(cmdlist);
	port_desc->clbu = HI32(cmdlist);

    port_desc->fb   = LO32(fis);
	port_desc->fbu  = HI32(fis);


    return result;
}

void ahci_port_power_on(ahci_port_t* port){
	port->port_reg->cmd |= (1 << 2);
}

void ahci_port_spin_up(ahci_port_t* port){
	port->port_reg->cmd |= (1 << 1);
}

void ahci_port_activate_link(ahci_port_t* port){
    port->port_reg->cmd = (port->port_reg->cmd & ~(0xf << 28)) | (1 << 28);
}

void ahci_enable_fis_receive_enable(ahci_port_t* port){
    port->port_reg->cmd |= (1 << 4);
}