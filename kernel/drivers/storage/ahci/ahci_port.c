#include "ahci_port.h"
#include "memory/pmm.h"
#include "string.h"
#include "memory/paging.h"
#include "io.h"

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

ahci_port_t* initialize_port(ahci_port_t* port, uint32_t index, HBA_PORT* port_desc){
    ahci_device_type device_type = get_device_type(port_desc);
	if(device_type == 0)
		return NULL;
		
    ahci_port_t* result = port;
    memset(result, 0, sizeof(ahci_port_t));

    result->port_reg = port_desc;
    result->index = index;
    result->device_type = device_type;

    void* port_mem = (void*)alloc_page();
    memset(port_mem, 0, 4096);
    result->command_list = port_mem;
    result->fis = port_mem + sizeof(HBA_COMMAND) * COMMAND_LIST_ENTRY_COUNT;
    //Записать адрес списка команд
    port_desc->clb  = LO32(result->command_list);
	port_desc->clbu = HI32(result->command_list);
    //Записать адрес структуры FIS
    port_desc->fb   = LO32(result->fis);
	port_desc->fbu  = HI32(result->fis);

    size_t cmd_tables_mem_sz = COMMAND_LIST_ENTRY_COUNT * sizeof(HBA_COMMAND_TABLE);
    uint32_t pages = cmd_tables_mem_sz / PAGE_SIZE;

    HBA_COMMAND_TABLE* cmd_tables_mem = (HBA_COMMAND_TABLE*)alloc_pages(pages);
    memset(cmd_tables_mem, 0, sizeof(ahci_port_t));

    for(int i = 0; i < 32; i ++){
        result->command_list[i].prdtl = 8; //8 PRDT на каждую команду
        HBA_COMMAND_TABLE* cmd_table = &cmd_tables_mem[i];
        //Записать адрес таблицы
        result->command_list[i].ctdba_low = LO32(cmd_table);
		result->command_list[i].ctdba_up = HI32(cmd_table);
    }
    //Очистить бит IRQ статуса
    port->port_reg->is = port->port_reg->is;

    ahci_port_clear_error_register(port);

    ahci_port_power_on(port);

    ahci_port_spin_up(port);

    ahci_port_activate_link(port);

    ahci_enable_fis_receive_enable(port);

    ahci_port_flush_posted_writes(port);

    if ((port->port_reg->tfd & 0xff) == 0xff)
		io_delay(200000);

	if ((port->port_reg->tfd & 0xff) == 0xff) {
		printf("%s: port %d: invalid task file status 0xff\n", __func__, index);
		return NULL;
	}

		switch (port->port_reg->ssts & HBA_PORT_SPD_MASK) {
			case 0x10:
				port->speed = AHCI_PORT_SPEED_1_5_GBPS;
				break;
			case 0x20:
				port->speed = AHCI_PORT_SPEED_3_0_GBPS;
				break;
			case 0x30:
				port->speed = AHCI_PORT_SPEED_6_0_GBPS;
				break;
			default:
				port->speed = AHCI_PORT_SPEED_UNRESTRICTED;
				break;
		}
	

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

void ahci_port_clear_error_register(ahci_port_t* port)
{
	port->port_reg->serr = port->port_reg->serr;
	ahci_port_flush_posted_writes(port);
}