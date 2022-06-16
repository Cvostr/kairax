#include "ahci_port.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/paging.h"
#include "io.h"
#include "ctype.h"

#define LO32(val) ((uint32_t)(uint64_t)(val))
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
	ahci_port_t* result = port;
    memset(result, 0, sizeof(ahci_port_t));
	port->implemented = 1;

    ahci_device_type device_type = get_device_type(port_desc);
	if(device_type == 0)
		return NULL;
		
    result->port_reg = port_desc;
    result->index = index;
    result->device_type = device_type;

	void* port_mem_v = get_first_free_pages(get_kernel_pml4(), 1);
    void* port_mem = (void*)pmm_alloc_page();
    memset(port_mem, 0, 4096);
	map_page_mem(get_kernel_pml4(), port_mem_v, port_mem, PAGE_WRITABLE | PAGE_PRESENT | PAGE_UNCACHED);
	port_mem = port_mem_v;
	//void* port_mem_v = kmalloc(4096);
	//memset(port_mem_v, 0, 4096);
	//void* port_mem = kheap_get_phys_address(port_mem_v);

    result->command_list = port_mem;
    result->fis = port_mem + sizeof(HBA_COMMAND) * COMMAND_LIST_ENTRY_COUNT;
    //Записать адрес списка команд
    port_desc->clb  = LO32(result->command_list);
	port_desc->clbu = HI32(result->command_list);
    //Записать адрес структуры FIS
    port_desc->fb   = LO32(result->fis);
	port_desc->fbu  = HI32(result->fis);

    size_t cmd_tables_mem_sz = COMMAND_LIST_ENTRY_COUNT * sizeof(HBA_COMMAND_TABLE);
    uint32_t pages = cmd_tables_mem_sz / PAGE_SIZE + 1;

	HBA_COMMAND_TABLE* cmd_tables_mem_v = get_first_free_pages(get_kernel_pml4(), pages);
    HBA_COMMAND_TABLE* cmd_tables_mem = (HBA_COMMAND_TABLE*)pmm_alloc_pages(pages);
    memset(cmd_tables_mem, 0, pages * PAGE_SIZE	);
	map_page_mem(get_kernel_pml4(), cmd_tables_mem_v, cmd_tables_mem, PAGE_WRITABLE | PAGE_PRESENT | PAGE_UNCACHED);
	//cmd_tables_mem = cmd_tables_mem_v;

    for(int i = 0; i < 32; i ++){
        result->command_list[i].prdtl = 8; //8 PRDT на каждую команду
        HBA_COMMAND_TABLE* cmd_table = &cmd_tables_mem[i];
        //Записать адрес таблицы
        result->command_list[i].ctdba_low = LO32(cmd_table);
		result->command_list[i].ctdba_up = HI32(cmd_table);
    }

	while(port->port_reg->cmd & HBA_PxCMD_CR);

    //Очистить бит IRQ статуса
    port->port_reg->is = port->port_reg->is;

    ahci_port_clear_error_register(port);

    ahci_port_power_on(port);

    ahci_port_activate_link(port);

    ahci_port_fis_receive_enable(port);

	ahci_port_spin_up(port);

    ahci_port_flush_posted_writes(port);

    if ((port->port_reg->tfd & 0xff) == 0xff)
		io_delay(200000);

	if ((port->port_reg->tfd & 0xff) == 0xff) {
		printf("%s: port %i: invalid task file status 0xff\n", __func__, index);
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

uint32_t ahci_port_get_free_cmdslot(ahci_port_t* port){
	uint32_t slots = (port->port_reg->sact | port->port_reg->ci);
	for(uint32_t i = 0; i < 32; i ++){
		if((slots & 1) == 0)
			return i;
		slots >>= 1;
	}
	return -1;
}

void ahci_port_power_on(ahci_port_t* port){
	port->port_reg->cmd |= (1 << 2);
}

void ahci_port_spin_up(ahci_port_t* port){
	port->port_reg->cmd |= HBA_PxCMD_START;
}

void ahci_port_activate_link(ahci_port_t* port){
    port->port_reg->cmd = (port->port_reg->cmd & ~(0xf << 28)) | (1 << 28);
}

void ahci_port_fis_receive_enable(ahci_port_t* port){
    port->port_reg->cmd |= HBA_PxCMD_FRE;
}

void ahci_port_clear_error_register(ahci_port_t* port)
{
	port->port_reg->serr = port->port_reg->serr;
	ahci_port_flush_posted_writes(port);
}

int ahci_port_identity(ahci_port_t *port, char* buffer){
	memset(buffer, 0, 512);

	HBA_PORT* hba_port = port->port_reg;
	hba_port->is = (uint32_t) -1;		// Clear pending interrupt bits
	int spin = 0; 
	int slot = ahci_port_get_free_cmdslot(port);
	if (slot == -1)
		return 0;

	HBA_COMMAND *cmdheader = (HBA_COMMAND*)hba_port->clb;
	cmdheader += slot;
	cmdheader->cmd_fis_len = sizeof(FIS_HOST_TO_DEV) / sizeof(uint32_t);	// Размер FIS таблицы
	cmdheader->write = 0;		// Чтение с диска
	cmdheader->prdtl = 1;		// количество PRDT
	cmdheader->prefetchable = 1;
	cmdheader->prdbc = 512;

	HBA_COMMAND_TABLE *cmdtbl = (HBA_COMMAND_TABLE*)(cmdheader->ctdba_low);
	memset(cmdtbl, 0, sizeof(HBA_COMMAND_TABLE));


	cmdtbl->prdt_entry[0].dba = (uint32_t)V2P(buffer);
	cmdtbl->prdt_entry[0].dbau = 0;
	cmdtbl->prdt_entry[0].dbc = 512 - 1;
	cmdtbl->prdt_entry[0].i = 1;


	FIS_HOST_TO_DEV *cmdfis = (FIS_HOST_TO_DEV*)(&cmdtbl->cfis);
	memset(cmdfis, 0, sizeof(FIS_HOST_TO_DEV));
 
	cmdfis->fis_type = FIS_TYPE_REG_HOST_TO_DEV;
	cmdfis->cmd_ctrl = 1;							// Команда
	cmdfis->command = FIS_CMD_IDENTIFY_DEVICE;		// Команда идентификации
	cmdfis->device = 0;
	

	while ((hba_port->tfd & (AHCI_DEV_BUSY | AHCI_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		printf("Port is hung\n");
		return 0;
	}

	hba_port->ci = (1 << slot);	// Issue command

	while (1)
	{
		if ((hba_port->ci & (1 << slot)) == 0) 
			break;
		if (hba_port->is & HBA_PxIS_TFE)	// Task file error
		{
			printf("Disk identity error\n");
			return 0;
		}
	}
}

uint32_t parse_identity_buffer(char* buffer,
							   uint16_t* device_type,
							   uint16_t* capabilities,
							   uint32_t* cmd_sets,
							   uint32_t* size,
							   char* model,
							   char* serial)
{
	*device_type = *((uint16_t*)(buffer + ATA_IDENT_DEVICETYPE));
	*capabilities = *((uint16_t*)(buffer + ATA_IDENT_CAPABILITIES));
	*cmd_sets =  *((uint32_t *)(buffer + ATA_IDENT_COMMANDSETS));

	if (*cmd_sets & (1 << 26))
        // 48 битный LBA
        *size = *((uint32_t *)(buffer + ATA_IDENT_MAX_LBA_EXT));
    else
        // 28 битный CHS
        *size = *((uint32_t *)(buffer + ATA_IDENT_MAX_LBA));
	//Копирование модели диска (каждая буква в модели поменяна с соседней местами)
	for (int k = 0; k < 40; k += 2) {
        model[k] = buffer[ATA_IDENT_MODEL + k + 1];
        model[k + 1] = buffer[ATA_IDENT_MODEL + k];
    }
	//Копирование серийного номера
	for (int k = 0; k < 34; k += 2) {
        serial[k] = buffer[ATA_IDENT_SERIAL + k + 1];
        serial[k + 1] = buffer[ATA_IDENT_SERIAL + k];
    }
	//удалить пробелы для модели
	for(int k = 39; k > 0; k --){
		if(model[k] == ' ')
			model[k] = '\0';
		else 
			break;
	}
	//удалить пробелы для серийного номера
	for(int p = strlen(serial) - 1; p > 0; p --){
		if(serial[p] == ' ')
			serial[p] = '\0';
		else 
			break;
	}
}

int ahci_port_read_lba(ahci_port_t *port, uint64_t start, uint64_t count, uint16_t *buf){
	return ahci_port_read_lba48(port, start, (uint32_t)count, buf);
}

int ahci_port_read_lba48(ahci_port_t *port, uint64_t start, uint32_t count, uint16_t *buf){
	HBA_PORT* hba_port = port->port_reg;
	hba_port->is = (uint32_t) -1;		// Clear pending interrupt bits
	int spin = 0; 
	int slot = ahci_port_get_free_cmdslot(port);
	if (slot == -1)
		return 0;

	HBA_COMMAND *cmdheader = (HBA_COMMAND*)hba_port->clb;
	cmdheader += slot;
	cmdheader->cmd_fis_len = sizeof(FIS_HOST_TO_DEV) / sizeof(uint32_t);	// Размер FIS таблицы
	cmdheader->write = 0;									// Чтение с диска
	cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1;	// необходимое количество PRDT
	
	HBA_COMMAND_TABLE *cmdtbl = (HBA_COMMAND_TABLE*)(cmdheader->ctdba_low);
	memset(cmdtbl, 0, sizeof(HBA_COMMAND_TABLE) +
 		(cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));
	
	// 8K bytes (16 sectors) per PRDT
	for (int i = 0; i < cmdheader->prdtl - 1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (uint32_t) buf;
		cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4 * 1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	
	// Last entry
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dba = (uint32_t) buf;
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dbc = (count << 9)-1;	// 512 bytes per sector
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].i = 1;
	
	// Setup command
	FIS_HOST_TO_DEV *cmdfis = (FIS_HOST_TO_DEV*)(&cmdtbl->cfis);
 
	cmdfis->fis_type = FIS_TYPE_REG_HOST_TO_DEV;
	cmdfis->cmd_ctrl = 1;	// Команда, а не управление
	cmdfis->command = FIS_CMD_READ_DMA_EXT;
 
	cmdfis->lba[0] = (uint8_t)start;
	cmdfis->lba[1] = (uint8_t)(start >> 8);
	cmdfis->lba[2] = (uint8_t)(start >> 16);
	cmdfis->device = (1 << 6);	// режим LBA
 
	cmdfis->lba2[0] = (uint8_t)(start >> 24);
	cmdfis->lba2[1] = (uint8_t)(start >> 32);
	cmdfis->lba2[2] = (uint8_t)(start >> 40);
 
	cmdfis->count_l = count & 0xFF;
	cmdfis->count_h = (count >> 8) & 0xFF;

	// The below loop waits until the port is no longer busy before issuing a new command
	while ((hba_port->tfd & (AHCI_DEV_BUSY | AHCI_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		printf("Port is hung\n");
		return 0;
	}
	hba_port->ci = (1 << slot);	// Issue command
 
	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((hba_port->ci & (1 << slot)) == 0) 
			break;
		if (hba_port->is & HBA_PxIS_TFE)	// Task file error
		{
			printf("Read disk error (%i, %i)\n", start, count);
			return 0;
		}
	}
 
	// Check again
	if (hba_port->is & HBA_PxIS_TFE)
	{
		printf("Read disk error (%i, %i)\n", start, count);
		return 0;
	}

	return 1;
}

int ahci_port_write_lba(ahci_port_t *port, uint64_t start, uint64_t count, uint16_t *buf){
	return ahci_port_write_lba48(port, (uint32_t)start, (uint32_t)(start >> 32), (uint32_t)count, buf);
}

int ahci_port_write_lba48(ahci_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf){
	HBA_PORT* hba_port = port->port_reg;
	hba_port->is = (uint32_t) -1;		// Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int slot = ahci_port_get_free_cmdslot(port);
	if (slot == -1)
		return 0;

	HBA_COMMAND *cmdheader = (HBA_COMMAND*)hba_port->clb;
	cmdheader += slot;
	cmdheader->cmd_fis_len = sizeof(FIS_HOST_TO_DEV) / sizeof(uint32_t);	// Command FIS size
	cmdheader->write = 1;									// Запись на устройство
	cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1;	// PRDT entries count
	
	HBA_COMMAND_TABLE *cmdtbl = (HBA_COMMAND_TABLE*)(cmdheader->ctdba_low);
	memset(cmdtbl, 0, sizeof(HBA_COMMAND_TABLE) +
 		(cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));
	
	// 8K bytes (16 sectors) per PRDT
	for (int i = 0; i < cmdheader->prdtl - 1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (uint32_t) buf;
		cmdtbl->prdt_entry[i].dbc = 8 * 1024-1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4 * 1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	
	// Last entry
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dba = (uint32_t) buf;
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dbc = (count<<9)-1;	// 512 bytes per sector
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].i = 1;
	
	// Setup command
	FIS_HOST_TO_DEV *cmdfis = (FIS_HOST_TO_DEV*)(&cmdtbl->cfis);
 
	cmdfis->fis_type = FIS_TYPE_REG_HOST_TO_DEV;
	cmdfis->cmd_ctrl = 1;	// Command
	cmdfis->command = FIS_CMD_WRITE_DMA_EXT;
 
	cmdfis->lba[0] = (uint8_t)startl;
	cmdfis->lba[1] = (uint8_t)(startl>>8);
	cmdfis->lba[2] = (uint8_t)(startl>>16);
	cmdfis->device = (1 << 6);	// LBA mode
 
	cmdfis->lba2[0] = (uint8_t)(startl>>24);
	cmdfis->lba2[1] = (uint8_t)starth;
	cmdfis->lba2[2] = (uint8_t)(starth>>8);
 
	cmdfis->count_l = count & 0xFF;
	cmdfis->count_h = (count >> 8) & 0xFF;

	// The below loop waits until the port is no longer busy before issuing a new command
	while ((hba_port->tfd & (AHCI_DEV_BUSY | AHCI_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		printf("Port is hung\n");
		return 0;
	}
	hba_port->ci = (1 << slot);	// Issue command
 
	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((hba_port->ci & (1 << slot)) == 0) 
			break;
		if (hba_port->is & HBA_PxIS_TFE)	// Task file error
		{
			printf("Write disk error\n");
			return 0;
		}
	}
 
	// Check again
	if (hba_port->is & HBA_PxIS_TFE)
	{
		printf("Read disk error\n");
		return 0;
	}
 
	return 1;
}