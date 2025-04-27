#include "dev/bus/pci/pci.h"
#include "usb.h"
#include "usb_xhci.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "string.h"
#include "stdio.h"
#include "kstdlib.h"

#define XHCI_CRCR_ENTRIES 256
#define XHCI_EVENT_RING_ENTIRIES 256

int xhci_device_probe(struct device *dev) 
{
	int rc;
	struct pci_device_info* device_desc = dev->pci_info;

	printk("XHCI controller found on bus: %i, device: %i func: %i\n", 
		device_desc->bus,
		device_desc->device,
		device_desc->function);

	pci_set_command_reg(dev->pci_info, 
		pci_get_command_reg(dev->pci_info) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE | ~PCI_DEVCMD_IO_ENABLE);
    pci_device_set_enable_interrupts(dev->pci_info, 1);

	if (pci_device_is_msi_capable(dev->pci_info) == 0) 
	{
		printk("XHCI: Controller does not support MSI interrupts\n");
		return -1;
	}

	struct xhci_controller* cntrl = kmalloc(sizeof(struct xhci_controller));
	memset(cntrl, 0, sizeof(struct xhci_controller));

	cntrl->mmio_addr_phys = dev->pci_info->BAR[0].address;
    cntrl->mmio_addr = map_io_region(cntrl->mmio_addr_phys, dev->pci_info->BAR[0].size);

	// указатели на основные структуры
	cntrl->cap = (struct xhci_cap_regs*) cntrl->mmio_addr;
	cntrl->op = (struct xhci_op_regs*) (cntrl->mmio_addr + cntrl->cap->caplen);
	cntrl->ports_regs = cntrl->op->portregs;
	cntrl->runtime = (struct xhci_runtime_regs*) (cntrl->mmio_addr + (cntrl->cap->rtsoff & ~0x1Fu));
	cntrl->doorbell = (union xhci_doorbell_register*) (cntrl->mmio_addr + (cntrl->cap->dboff & ~0x3));

	// Получение свойств
	cntrl->slots = cntrl->cap->hcsparams1 & 0xFF;
	cntrl->ports_num = cntrl->cap->hcsparams1 >> 24;
	cntrl->interrupters = (cntrl->cap->hcsparams1 >> 8) & 0b11111111111;
	printk("XHCI: caplen: %i version: %i, slots: %i, ports: %i, ints: %i\n", (cntrl->cap->caplen), (cntrl->cap->version) & 0xFFFF, cntrl->slots, cntrl->ports_num, cntrl->interrupters);

	cntrl->ports = kmalloc(cntrl->ports_num * sizeof(struct xhci_port_desc));

	// Остановка и сброс контроллера
	xhci_controller_stop(cntrl);
	hpet_sleep(1);
	if (rc = xhci_controller_reset(cntrl))
	{
		printk("XHCI: Error during controller reset: %i\n", rc);
        return -1;
	}

	cntrl->max_scratchpad_buffers = (((cntrl->cap->hcsparams2 >> 21) & 0x1F) << 5) | ((cntrl->cap->hcsparams2 >> 27) & 0x1F);
	cntrl->ext_cap_offset = (cntrl->cap->hccparams1 >> 16) & UINT16_MAX;
	cntrl->pagesize = 1ULL << (cntrl->op->pagesize + 12);
	printk("XHCI: scratchpad: %i, pagesize: %i, ext cap offset: %i, runt offset: %i\n", cntrl->max_scratchpad_buffers, cntrl->pagesize, cntrl->ext_cap_offset, cntrl->cap->rtsoff);

	if (cntrl->ext_cap_offset == 0)
	{
		printk("XHCI: Controller doesnt have extended capabilities\n");
		unmap_io_region(cntrl->mmio_addr_phys, dev->pci_info->BAR[0].size);
		kfree(cntrl);
		return -1;
	}

	// Device Notifications Enable
	cntrl->op->dnctrl = 0xFFFF;

	xhci_controller_check_ext_caps(cntrl);

	// DCBAA хранит адреса на контекстную структуру каждого слота
	size_t dcbaa_size = (cntrl->slots + 1) * sizeof(void*);
	// Выделить память
	uintptr_t dcbaa_phys = (uintptr_t) pmm_alloc(dcbaa_size, NULL);
	cntrl->dcbaa = map_io_region(dcbaa_phys, dcbaa_size);
	memset(cntrl->dcbaa, 0, dcbaa_size);
	// Записать адрес в регистр DCBAA
	cntrl->op->dcbaa = dcbaa_phys;

	// Создать Command Ring
	cntrl->cmdring = xhci_create_command_ring(XHCI_CRCR_ENTRIES);
	// Записать адрес в регистр CRCR
	cntrl->op->crcr = cntrl->cmdring->trbs_phys | cntrl->cmdring->cycle_bit;

	// Выделим память под кучу записей Event Ring Segment Table Entry
	cntrl->event_ring = xhci_create_event_ring(XHCI_EVENT_RING_ENTIRIES, 1);
	
	uint8_t irq = alloc_irq(0, "xhci");
    printk("XHCI: using IRQ %i\n", irq);
    rc = pci_device_set_msi_vector(dev, irq);
    if (rc == -1)
	{
        printk("XHCI: Error assigning MSI IRQ vector: %i\n", rc);
        return -1;
    }
	register_irq_handler(irq, xhci_int_hander, cntrl);

	xhci_controller_init_interrupts(cntrl, cntrl->event_ring);
	xhci_controller_init_scratchpad(cntrl);
	
	if ((rc = xhci_controller_start(cntrl)) != 0)
	{
		printk("XHCI: Error on start: %i\n", rc);
		// TODO: Clear everything
		return -1;
	}
	// Сохранить указатель на структуру
	dev->dev_data = cntrl;

	/*
	struct xhci_trb test_trb = {0,};
	test_trb.interrupt_on_completion = 1;
	test_trb.type = XHCI_TRB_NO_OP_CMD;

	xhci_command_enqueue(cntrl->cmdring, &test_trb);
	cntrl->doorbell[0].doorbell = 0;*/

	//while (1) {
	//xhci_controller_init_ports(cntrl);
	//}

    return 0;
}

void xhci_controller_init_ports(struct xhci_controller* controller)
{
	for (size_t i = 0; i < controller->ports_num; i ++)
	{
		struct xhci_port_desc* port = &controller->ports[i];
		port->port_regs = &controller->ports_regs[i];
		//if (port->revision_major == 0)
		//	continue;

		if (!(port->port_regs->status & XHCI_PORTSC_PORTPOWER))
			continue;

		port->port_regs->status = XHCI_PORTSC_CSC | XHCI_PORTSC_PRC | XHCI_PORTSC_PORTPOWER;

		hpet_sleep(20);

		if (!(port->port_regs->status & XHCI_PORTSC_CCS))
		{
			printk("ERR ");
		} else {
			printk("SUCCESS ");
		}
	}
}

// eXtensible Host Controller Interface for Universal Serial Bus (xHCI) (4.20)
int xhci_controller_init_scratchpad(struct xhci_controller* controller)
{
	// Рассчитаем размер буфера
	size_t scratchpad_size = controller->max_scratchpad_buffers * sizeof(uintptr_t);
	// Выделим память под основной буфер
	uintptr_t scratchpad_phys = (uintptr_t) pmm_alloc(scratchpad_size, NULL);
	uintptr_t* scratchpad = map_io_region(scratchpad_phys, scratchpad_size);
	
	// Переведем размер XHCI страницы в количество системных страниц
	uint32_t xhci_page_in_sys_pages = controller->pagesize / PAGE_SIZE;

	// Заполним буфер адресами выделяемых страниц
	for (uint32_t i = 0; i < scratchpad_size; i ++)
	{
		uintptr_t phys = pmm_alloc_pages(xhci_page_in_sys_pages);
		scratchpad[i] = phys;
	}

	// Адрес на scratchpad буфер записываем в начало DCBAA
	controller->dcbaa[0] = scratchpad_phys;
}

int xhci_controller_init_interrupts(struct xhci_controller* controller, struct xhci_event_ring* event_ring)
{
	struct xhci_interrupter* interrupter = &controller->runtime->interrupters[0];
	interrupter->erstba = event_ring->segment_table_phys;
	interrupter->erstsz = event_ring->segment_count;

	uintptr_t trb_offset = event_ring->trbs_phys + event_ring->next_trb_index * sizeof(struct xhci_trb);
	interrupter->erdp = trb_offset | XHCI_EVENT_HANDLER_BUSY;

	// Включить прерывания на этом interrupter
	interrupter->iman = interrupter->iman | XHCI_IMAN_INTERRUPT_PENDING | XHCI_IMAN_INTERRUPT_ENABLE;

	// Сбросить флаг прерывания (если было необработанное прерывание)
	controller->op->usbsts |= XHCI_STS_EINTERRUPT;
}

struct xhci_command_ring *xhci_create_command_ring(size_t ntrbs)
{
	struct xhci_command_ring *cmdring = kmalloc(sizeof(struct xhci_command_ring));
	cmdring->trbs_num = ntrbs;
	cmdring->cycle_bit = XHCI_CR_CYCLE_STATE;
	cmdring->next_trb_index = 0;

	size_t cmd_ring_sz = ntrbs * sizeof(struct xhci_trb);
	cmdring->trbs_phys = (uintptr_t) pmm_alloc(cmd_ring_sz, NULL);
	cmdring->trbs = (struct xhci_trb*) map_io_region(cmdring->trbs_phys, cmd_ring_sz);
	memset(cmdring->trbs, 0, cmd_ring_sz);

	// Установить последнее TRB как ссылку на первое
	cmdring->trbs[ntrbs - 1].type = XHCI_TRB_TYPE_LINK;
	cmdring->trbs[ntrbs - 1].link.cycle = cmdring->cycle_bit;
	cmdring->trbs[ntrbs - 1].link.cycle_enable = 1;
	cmdring->trbs[ntrbs - 1].parameter = cmdring->trbs_phys;

	return cmdring;
}

void xhci_command_enqueue(struct xhci_command_ring *ring, struct xhci_trb* trb)
{
	// Запишем Cycle State (чтобы на следующем круге XHCI видел это как невыполненное)
	trb->cycle = ring->cycle_bit;

	memcpy(&ring->trbs[ring->next_trb_index], trb, sizeof(struct xhci_trb));

	if (++ring->next_trb_index == ring->trbs_num - 1)
	{
		// Достигли конечной TRB
		ring->trbs[ring->trbs_num - 1].link.cycle = ring->cycle_bit;
		// Сбрасываем указатель на 0
		ring->next_trb_index = 0;
		// Инвертируем Cycle State
		ring->cycle_bit = !ring->cycle_bit;
	}
}

struct xhci_event_ring *xhci_create_event_ring(size_t ntrbs, size_t segments)
{
	struct xhci_event_ring *eventring = kmalloc(sizeof(struct xhci_event_ring));
	eventring->trb_count = ntrbs;
	eventring->segment_count = segments;
	eventring->cycle_bit = XHCI_CR_CYCLE_STATE;
	eventring->next_trb_index = 0;

	size_t event_ring_buffer_size = ntrbs * sizeof(struct xhci_trb);
	// Выделить память
	eventring->trbs_phys = (uintptr_t) pmm_alloc(event_ring_buffer_size, NULL);
	eventring->trbs = map_io_region(eventring->trbs_phys, event_ring_buffer_size);
	memset(eventring->trbs, 0, event_ring_buffer_size);

	// Выделим память под кучу записей Event Ring Segment Table Entry
	size_t segment_table_buffer_size = eventring->segment_count * sizeof(struct xhci_event_ring_seg_table_entry);
	eventring->segment_table_phys = (uintptr_t) pmm_alloc(segment_table_buffer_size, NULL);
	eventring->segment_table = map_io_region(eventring->segment_table_phys, segment_table_buffer_size);
	memset(eventring->segment_table, 0, segment_table_buffer_size);

	// настроим только первый элемент
	struct xhci_event_ring_seg_table_entry* primary_entry = &eventring->segment_table[0];
	primary_entry->rsba = eventring->trbs_phys;
	primary_entry->rsz = ntrbs;
	primary_entry->rsvd = 0;
}

int xhci_event_ring_deque(struct xhci_event_ring *ring, struct xhci_trb *trb)
{
	struct xhci_trb* dequed = &ring->trbs[ring->next_trb_index];
	if (dequed->cycle != ring->cycle_bit)
	{
		return 0;
	}

	memcpy(trb, dequed, sizeof(struct xhci_trb));

	if (++ring->next_trb_index >= ring->trb_count)
	{
		ring->next_trb_index = 0;
		ring->cycle_bit = !ring->cycle_bit;
	}
}

void xhci_controller_stop(struct xhci_controller* controller)
{
	uint32_t cmd = controller->op->usbcmd;
	cmd &= ~XHCI_CMD_RUN;
	controller->op->usbcmd = cmd;

	while (!(controller->op->usbsts & XHCI_STS_HALT));
}

int xhci_controller_reset(struct xhci_controller* controller)
{
	uint32_t cmd = controller->op->usbcmd;
	cmd |= XHCI_CMD_RESET;
	controller->op->usbcmd = cmd;

	// Ожидаем, пока биты не пропали 
	while ((controller->op->usbcmd & XHCI_CMD_RESET) || (controller->op->usbsts & XHCI_STS_NOT_READY));

	// Советуют подождать 50 мс
	hpet_sleep(50);

	// Проверяем, что все базовые регистры занулены
	if ((controller->op->usbcmd != 0) ||
		(controller->op->dnctrl != 0) ||
		(controller->op->crcr != 0) ||
		(controller->op->dcbaa != 0)
	) 
	{
		return -10;
	}

	return 0;
}

int xhci_controller_start(struct xhci_controller* controller)
{
	// Включить контроллер вместе с прерываниями
	controller->op->usbcmd |= (XHCI_CMD_RUN | XHCI_CMD_INTE | XHCI_CMD_HSEE);

	int tries = 20;
	while (tries > 0 && (controller->op->usbsts & XHCI_STS_HALT))
	{
		hpet_sleep(1);
	}
	if ((controller->op->usbsts & XHCI_STS_HALT))
	{
		return -1;
	}
	if ((controller->op->usbsts & XHCI_STS_NOT_READY) == XHCI_STS_NOT_READY)
	{
		return -2;
	}

	return 0;
}

int xhci_controller_check_ext_caps(struct xhci_controller* controller)
{
	char* ext_cap_ptr = controller->mmio_addr + controller->ext_cap_offset * sizeof(uint32_t);

	while (1)
	{
		struct xhci_extended_cap* cap = (struct xhci_extended_cap*) ext_cap_ptr;
		//printk("XHCI: CAP ID %i\n", cap->capability_id);

		if (cap->capability_id == XHCI_EXT_CAP_SUPPORTED_PROTOCOL)
		{
			struct xhci_protocol_cap* prot_cap = (struct xhci_protocol_cap*) ext_cap_ptr;
			const uint32_t target_name_string =
				('U' <<  0) |
				('S' <<  8) |
				('B' << 16) |
				(' ' << 24);

			if (prot_cap->name_string != target_name_string)
			{
				printk("Invalid port protocol name string\n");
				return -1;
			}

			if (prot_cap->compatible_port_offset == 0)
			{
				printk("Invalid port protocol offset\n");
				return -1;
			}

			printk("XHCI: compatible port count %i\n", prot_cap->compatible_port_count);
			for (size_t i = 0; i < prot_cap->compatible_port_count; i++)
			{
				struct xhci_port_desc* port = &controller->ports[prot_cap->compatible_port_offset + i - 1];
				port->proto_cap = prot_cap;
				//port->revision_major = prot_cap->major_revision;
				//port->revision_minor = prot_cap->minor_revision;
				//printk("XHCI: port %i, revision major %i, revision minor %i\n", i, port->revision_major, port->revision_minor);
			}
		} else if (cap->capability_id == XHCI_EXT_CAP_LEGACY_SUPPORT)
		{
			printk("XHCI: legacy support capability\n");

			struct xhci_legacy_support_cap* legacy_cap = (struct xhci_legacy_support_cap*) ext_cap_ptr;
			
			if (legacy_cap->bios_owned_semaphore) 
			{
				printk("XHCI: Performing handoff\n");
				legacy_cap->os_owned_semaphore = 1;
			}

			// TODO: add semaphore
			while (legacy_cap->bios_owned_semaphore);
		}

		if (cap->next_capability > 0)
		{
			ext_cap_ptr += cap->next_capability * sizeof(uint32_t);
		} 
		else
		{
			break;
		}
	}

	controller->op->config.max_device_slots_enabled = controller->slots;

	return 0;
}

void xhci_int_hander(void* regs, struct xhci_controller* data) 
{
	if (!(data->op->usbsts & XHCI_STS_EINTERRUPT))
		return;

	printk("XHCI: interrupt\n");

	data->op->usbsts = XHCI_STS_EINTERRUPT;
}

struct pci_device_id xhci_ctrl_ids[] = {
	{PCI_DEVICE_CLASS(0xC, 0x3, 0x30)},
	{0,}
};

struct device_driver_ops usb3_ctrl_ops = {

    .probe = xhci_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_driver xhci_ctrl_driver = {
	.dev_name = "USB 3.0 (XHCI) Host Controller",
	.pci_device_ids = xhci_ctrl_ids,
	.ops = &usb3_ctrl_ops
};

void usb_init()
{
	register_pci_device_driver(&xhci_ctrl_driver);

}
