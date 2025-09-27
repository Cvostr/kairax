#ifndef _PCI_H
#define _PCI_H

#include "kairax/types.h"

#define BAR_TYPE_MMIO	1
#define BAR_TYPE_IO		2

struct pci_device_bar {
	uintptr_t 		address;
	size_t 			size;
	uint32_t 		flags;
	int 			type;
} PACKED;

struct pci_device_info {
 	uint8_t			bus;
  	uint8_t 		device;

  	uint8_t			function;
  	uint16_t 		vendor_id;
  	uint16_t 		device_id;
	uint16_t		status;
  	uint8_t 		device_class;
  	uint8_t 		device_subclass;

  	uint8_t 		prog_if;
	uint8_t 		revision_id;

	uint8_t			cache_line_size;
	uint8_t 		latency_timer;

	uint8_t 		header_type;
	uint8_t 		bist;

	struct pci_device_bar		BAR[6];

	uint32_t 		cardbus_ptr;
} PACKED;

#define PCI_DEVCMD_BUSMASTER_ENABLE 0x4
#define PCI_DEVCMD_MSA_ENABLE 		0x2
#define PCI_DEVCMD_IO_ENABLE		0x1
#define PCI_DEVCMP_INTERRUPTS_DISABLE (1 << 10)

#define PCI_STATUS_INTERRUPT_STATUS			(1 << 3)
#define PCI_STATUS_CAPABILITIES_LIST		(1 << 4)
#define PCI_STATUS_66_MHZ_CAPABLE			(1 << 5)
#define PCI_STATUS_FAST_BACK_BACK_CAPABLE 	(1 << 7)
#define PCI_STATUS_MASTER_DATA_PATIRY_ERROR (1 << 8)

#define PCI_VENDOR_ID			0
#define PCI_PRODUCT_ID			2
#define PCI_CMD_REG				0x4
#define PCI_STATUS				6
#define PCI_DEVCLASS			0xA
#define PCI_CAPABILITIES_LIST	0x34
#define PCI_INTERRUPTS			0x3C

#define PCI_CAPABILITY_MSI		0x5
#define PCI_CAPABILITY_MSI_X	0x11

#define PCI_MSI_64BIT			(1 << 7)
#define PCI_MSI_PER_VECTOR_MASK	(1 << 8)

#define PCI_HEADER_TYPE_NORMAL	0
#define PCI_HEADER_TYPE_BRIDGE	0x01
#define PCI_HEADER_TYPE_CARDBUS	0x02
#define PCI_HEADER_TYPE_MULTI	0x80	// Многофункциональное устройство (уже встречали на XHCI)

struct msix_table_entry {
    uint64_t message_address;
    uint32_t message_data; 
    uint32_t vector_control;
} PACKED;

struct device;

uint16_t i_pci_config_read16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);
uint32_t i_pci_config_read32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);

uint16_t pci_config_read16(struct device* dev, uint32_t offset);
uint32_t pci_config_read32(struct device* dev, uint32_t offset);

void i_pci_config_write16(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint16_t data);
void i_pci_config_write32(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint32_t data);

void pci_config_write16(struct device* dev, uint32_t offset, uint16_t data);
void pci_config_write32(struct device* dev, uint32_t offset, uint32_t data);

uint16_t pci_get_command_reg(struct pci_device_info* device);

void pci_set_command_reg(struct pci_device_info* device, uint16_t flags);

// Включить или выключить прерывания
void pci_device_set_enable_interrupts(struct pci_device_info* device, int enable);

int pci_device_get_capability_register(struct pci_device_info* device, uint32_t capability, uint32_t *reg);
int pci_device_is_msi_capable(struct pci_device_info* device);
int pci_device_is_msix_capable(struct pci_device_info* device);
uint16_t pci_device_get_irq_line(struct pci_device_info* device);

// Назначить устройству номер MSI прерывания
// Включает MSI прерывания
int pci_device_set_msi_vector(struct device* device, uint32_t vector);

int pci_device_set_msix_vector(struct device* device, uint32_t vector);
void pci_device_clear_msix_pending_bit(char* pba_base, uint32_t vector);

// Попыпаться получить устройство по указанным параметрам
// Если устройство есть - будет зарегистрировано через register_device
int probe_pci_device(uint8_t bus, uint8_t device, uint8_t func);

// Сканировать шину PCI и добавить устройства
void load_pci_devices_list();

char* pci_get_device_name(uint8_t class, uint8_t subclass, uint8_t pif);

#endif
