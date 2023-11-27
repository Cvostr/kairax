#include "ahci_port.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/kernel_vmm.h"
#include "io.h"
#include "ctype.h"
#include "stdio.h"
#include "kstdlib.h"

#define LO32(val) ((uint32_t)(uint64_t)(val))
#define HI32(val) ((uint32_t)(((uint64_t)(val)) >> 32))

#define AHCI_INT_ON_COMPLETION 0

static int get_device_type(HBA_PORT *port)
{
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

void ahci_port_stop_commands(ahci_port_t* port)
{
	port->port_reg->cmd &= ~HBA_PxCMD_START;
	port->port_reg->cmd &= ~HBA_PxCMD_FRE;

	while(1) {
		
		if (port->port_reg->cmd & HBA_PxCMD_FR)
			continue;

		if (port->port_reg->cmd & HBA_PxCMD_CR)
			continue;

		break;
	}
}


ahci_port_t* initialize_port(ahci_port_t* port, uint32_t index, HBA_PORT* port_desc)
{
    memset(port, 0, sizeof(ahci_port_t));
		
	port->implemented = 1;
    port->port_reg = port_desc;
    port->index = index;
	
	// Чтобы избежать ошибок при конфигурации
	ahci_port_stop_commands(port);	

	// Выделение памяти под буфер порта
	char* port_mem = (char*)pmm_alloc_page();
	uint64_t pageFlags = PAGE_WRITABLE | PAGE_PRESENT | PAGE_UNCACHED;
	// Сменить флаги страницы, добавить PAGE_UNCACHED
	//set_page_flags(get_kernel_pml4(), (uintptr_t)P2V(port_mem), pageFlags);
	map_page_mem(get_kernel_pml4(),
			P2V(port_mem),
			port_mem,
			pageFlags);

    port->command_list = (HBA_COMMAND*)port_mem;
    port->fis = (fis_t*)(port_mem + sizeof(HBA_COMMAND) * COMMAND_LIST_ENTRY_COUNT);
    //Записать адрес списка команд
    port_desc->clb  = LO32(port->command_list);
	port_desc->clbu = HI32(port->command_list);
    //Записать адрес структуры FIS
    port_desc->fb   = LO32(port->fis);
	port_desc->fbu  = HI32(port->fis);

    size_t cmd_tables_mem_size = align(COMMAND_LIST_ENTRY_COUNT * sizeof(HBA_COMMAND_TABLE), PAGE_SIZE);
    uint32_t pages_num = cmd_tables_mem_size / PAGE_SIZE;

	//Выделить память под буфер команд
	char* cmd_tables_mem = (char*)pmm_alloc_pages(pages_num);
	// Сменить флаги страницы, добавить PAGE_UNCACHED
	for (int i = 0; i < pages_num; i ++) {
		//set_page_flags(get_kernel_pml4(), (uintptr_t)P2V(cmd_tables_mem) + i * PAGE_SIZE, pageFlags);
		map_page_mem(get_kernel_pml4(),
			P2V(cmd_tables_mem) + i * PAGE_SIZE,
			cmd_tables_mem + i * PAGE_SIZE,
			pageFlags);
	}

    for(int i = 0; i < COMMAND_LIST_ENTRY_COUNT; i ++){
		HBA_COMMAND* hba_command_virtual = (HBA_COMMAND*)P2V(port->command_list);
        hba_command_virtual[i].prdtl = 8; //8 PRDT на каждую команду
        HBA_COMMAND_TABLE* cmd_table = (HBA_COMMAND_TABLE*)cmd_tables_mem + i;
        //Записать адрес таблицы
        hba_command_virtual[i].ctdba_low = LO32(cmd_table);
		hba_command_virtual[i].ctdba_up = HI32(cmd_table);
    }

	port->port_reg->sctl |= (SCTL_PORT_IPM_NOPART | SCTL_PORT_IPM_NOSLUM);

	//Очистить бит IRQ статуса
    port->port_reg->is = port->port_reg->is;

	// Очистить регистр ошибок
	ahci_port_clear_error_register(port);

	ahci_port_power_on(port);

	ahci_port_spin_up(port);

	ahci_port_activate_link(port);

	// включает FIS Receive
	ahci_port_fis_receive_enable(port);

    ahci_port_flush_posted_writes(port);

    return port;
}

int ahci_port_init2(ahci_port_t* port)
{
	ahci_port_enable(port);

	// Включение прерываний
	port->port_reg->ie = PORT_INT_MASK;
	ahci_port_flush_posted_writes(port);

	// Reset device
	int reset_rc = ahci_port_reset(port);
	if (reset_rc == 0) {
		return 0;
	}

	if ((port->port_reg->tfd & 0xff) == 0xff)
		io_delay(200000);

	if ((port->port_reg->tfd & 0xff) == 0xff) {
		printf("%s: port %i: invalid task file status 0xff\n", __func__, port->index);
		return 0;
	}

	// Определение скорости
	uint32_t ssts = port->port_reg->ssts;
	switch (ssts & HBA_PORT_SPD_MASK) {
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

	while (port->port_reg->tfd & (AHCI_DEV_BUSY));

	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;
	if (det == HBA_PORT_DET_PRESENT && ipm == HBA_PORT_IPM_ACTIVE)	// Check drive status
		port->present = 1;
	else {
		return 0;
	}

	// Определение типа устройства
	port->device_type = get_device_type(port->port_reg);

	if (port->device_type == AHCI_DEV_SATAPI) {
		port->port_reg->cmd |= HBA_PxCMD_ATAPI;
	} else {
		port->port_reg->cmd &= ~HBA_PxCMD_ATAPI;
	}

	ahci_port_flush_posted_writes(port);

	return 1;
}

int ahci_port_enable(ahci_port_t* port)
{
	if ((port->port_reg->cmd & HBA_PxCMD_START) != 0) {
		printf("%s: Starting port already running!\n", __func__);
		return 0;
	}

	if ((port->port_reg->cmd & HBA_PxCMD_FRE) == 0) {
		printf("%s: Unable to start port without FRE enabled!\n", __func__);
		return 0;
	}

	// Ожидание
	int step = 0;
	while (port->port_reg->cmd & HBA_PxCMD_CR && step < 1000000) {
		step++;
	}
	if (step == 1000000) {
		return 0;
	}

	port->port_reg->cmd |= HBA_PxCMD_START;
	ahci_port_flush_posted_writes(port);

	return 1;
}

int ahci_port_reset(ahci_port_t* port)
{
	ahci_port_disable(port);
	ahci_port_clear_error_register(port);

	int spin = 0;
	while ((port->port_reg->tfd & (AHCI_DEV_BUSY | AHCI_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		printf("Port COMRESET\n");
		port->port_reg->sctl = (SSTS_PORT_IPM_ACTIVE | SSTS_PORT_IPM_PARTIAL | SCTL_PORT_DET_INIT);
		ahci_port_flush_posted_writes(port);

		io_delay(200000);
		port->port_reg->sctl = (port->port_reg->sctl & ~HBA_PORT_DET_MASK) | SCTL_PORT_DET_NOINIT;
		ahci_port_flush_posted_writes(port);
	}

	ahci_port_enable(port);

	spin = 0;
	while (!(port->port_reg->cmd & SSTS_PORT_DET_PRESENT) && spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		// Отсутствует диск в порту
		printf("NO DEVICE IN PORT\n");
		return 0;
	}

	return 1;
}

int ahci_port_interrupt(ahci_port_t *port)
{
	uint32_t is = port->port_reg->is;
	port->port_reg->is = is;
}

int ahci_port_disable(ahci_port_t* port)
{
	if ((port->port_reg->cmd & HBA_PxCMD_START) == 0) {
	} else {
		port->port_reg->cmd &= ~HBA_PxCMD_START;
		ahci_port_flush_posted_writes(port);
	}

	while (port->port_reg->cmd & HBA_PxCMD_CR);
}

uint32_t ahci_port_get_free_cmdslot(ahci_port_t* port)
{
	uint32_t slots = (port->port_reg->sact | port->port_reg->ci);

	for (uint32_t i = 0; i < COMMAND_LIST_ENTRY_COUNT; i ++) {

		if ((slots & 1) == 0) {
			return i;
		}

		slots >>= 1;
	}

	return -1;
}

void ahci_port_power_on(ahci_port_t* port)
{
	port->port_reg->cmd |= HBA_PxCMD_POWER;
}

void ahci_port_spin_up(ahci_port_t* port)
{
	port->port_reg->cmd |= HBA_PxCMD_SPINUP;
}

void ahci_port_activate_link(ahci_port_t* port)
{
    port->port_reg->cmd = (port->port_reg->cmd & ~AHCI_PORT_CMD_ICC_MASK) | AHCI_PORT_CMD_ICC_ACTIVE;
}

void ahci_port_fis_receive_enable(ahci_port_t* port)
{
    port->port_reg->cmd |= HBA_PxCMD_FRE;
}

void ahci_port_clear_error_register(ahci_port_t* port)
{
	port->port_reg->serr = port->port_reg->serr;
	ahci_port_flush_posted_writes(port);
}

int ahci_port_identity(ahci_port_t *port, char* buffer)
{
	HBA_PORT* hba_port = port->port_reg;
	hba_port->is = (uint32_t) -1;		// Clear pending interrupt bits
	int spin = 0; 
	int slot = ahci_port_get_free_cmdslot(port);
	if (slot == -1)
		return 0;

	HBA_COMMAND *cmdheader = (HBA_COMMAND*)P2V(((uintptr_t)hba_port->clbu << 32) | hba_port->clb);
	cmdheader += slot;
	cmdheader->cmd_fis_len = sizeof(FIS_HOST_TO_DEV) / sizeof(uint32_t);	// Размер FIS таблицы
	cmdheader->write = 0;		// Чтение с диска
	cmdheader->prdtl = 1;		// количество PRDT
	cmdheader->prefetchable = 1;
	cmdheader->prdbc = 512;

	HBA_COMMAND_TABLE *cmdtbl = (HBA_COMMAND_TABLE*)P2V(((uintptr_t)cmdheader->ctdba_up << 32) | cmdheader->ctdba_low);
	memset(cmdtbl, 0, sizeof(HBA_COMMAND_TABLE));

	cmdtbl->prdt_entry[0].dba = (uint32_t)((uintptr_t)buffer & 0xFFFFFFFF);
	cmdtbl->prdt_entry[0].dbau = (uint32_t)((uintptr_t)buffer >> 32);
	cmdtbl->prdt_entry[0].dbc = 512 - 1;
	cmdtbl->prdt_entry[0].i = AHCI_INT_ON_COMPLETION;

	FIS_HOST_TO_DEV *cmdfis = (FIS_HOST_TO_DEV*)(&cmdtbl->cfis);
	memset((void*)cmdfis, 0, sizeof(FIS_HOST_TO_DEV));
 
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

	hba_port->ci = (1 << slot);	// Выполнить команду

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

	HBA_COMMAND *cmdheader = (HBA_COMMAND*)P2V(((uintptr_t)hba_port->clbu << 32) | hba_port->clb);
	cmdheader += slot;
	cmdheader->cmd_fis_len = sizeof(FIS_HOST_TO_DEV) / sizeof(uint32_t);	// Размер FIS таблицы
	cmdheader->write = 0;									// Чтение с диска
	cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1;	// необходимое количество PRDT
	
	HBA_COMMAND_TABLE *cmdtbl = (HBA_COMMAND_TABLE*)P2V(((uintptr_t)cmdheader->ctdba_up << 32) | cmdheader->ctdba_low);
	memset(cmdtbl, 0, sizeof(HBA_COMMAND_TABLE) +
 		(cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));
	
	// 8K bytes (16 sectors) per PRDT
	for (int i = 0; i < cmdheader->prdtl - 1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (uint32_t) ((uintptr_t) buf & 0xFFFFFFFF);
		cmdtbl->prdt_entry[i].dbau = (uint32_t) ((uintptr_t) buf >> 32);
		cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = AHCI_INT_ON_COMPLETION;
		buf += 4 * 1024;	// 4K слов
		count -= 16;	// 16 секторов
	}
	
	// Last entry
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dba = (uint32_t) ((uintptr_t)buf & 0xFFFFFFFF);
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dbau = (uint32_t) ((uintptr_t)buf >> 32);
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dbc = (count << 9) - 1;	// 512 байт в секторе
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].i = AHCI_INT_ON_COMPLETION;
	
	// Подготовить команду
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

	hba_port->ci = (1 << slot);	// Выполнить команду
 
	// Ожидание завершения операции
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
	while (slot == -1)
		slot = ahci_port_get_free_cmdslot(port);

	HBA_COMMAND *cmdheader = (HBA_COMMAND*)P2V(((uintptr_t)hba_port->clbu << 32) | hba_port->clb);
	cmdheader += slot;
	cmdheader->cmd_fis_len = sizeof(FIS_HOST_TO_DEV) / sizeof(uint32_t);	// Command FIS size
	cmdheader->write = 1;									// Запись на устройство
	cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1;	// PRDT entries count
	
	HBA_COMMAND_TABLE *cmdtbl = (HBA_COMMAND_TABLE*)P2V(((uintptr_t)cmdheader->ctdba_up << 32) | cmdheader->ctdba_low);
	memset(cmdtbl, 0, sizeof(HBA_COMMAND_TABLE) +
 		(cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));
	
	// 8K bytes (16 sectors) per PRDT
	for (int i = 0; i < cmdheader->prdtl - 1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (uint32_t) ((uintptr_t) buf & 0xFFFFFFFF);
		cmdtbl->prdt_entry[i].dbau = (uint32_t) ((uintptr_t) buf >> 32);
		cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = AHCI_INT_ON_COMPLETION;
		buf += 4 * 1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	
	// Last entry
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dba = (uint32_t) ((uintptr_t) buf & 0xFFFFFFFF);
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dbau = (uint32_t) ((uintptr_t)buf >> 32);
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].dbc = (count << 9) - 1;	// 512 bytes per sector
	cmdtbl->prdt_entry[cmdheader->prdtl - 1].i = AHCI_INT_ON_COMPLETION;
	
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