#include "dev/bus/pci/pci.h"
#include "../usb_descriptors.h"
#include "usb_xhci.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/device_man.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "string.h"
#include "kairax/stdio.h"
#include "kstdlib.h"
#include "proc/process.h"
#include "proc/thread.h"

#define XHCI_CRCR_ENTRIES 256
#define XHCI_EVENT_RING_ENTIRIES 256

static const char* xhci_speed_string[7] = {
	"Invalid",
	"Full Speed (12 MB/s - USB2.0)",
	"Low Speed (1.5 Mb/s - USB 2.0)",
	"High Speed (480 Mb/s - USB 2.0)",
	"Super Speed (5 Gb/s - USB3.0)",
	"Super Speed Plus (10 Gb/s - USB 3.1)",
	"Undefined"
};

//#define XHCI_LOG_CMD_COMPLETION

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

	int msi_capable = pci_device_is_msi_capable(dev->pci_info);
	int msix_capable = pci_device_is_msix_capable(dev->pci_info);
		
	//printk("XHCI: MSI capable: %i MSI-X capable: %i\n", msi_capable, msix_capable);

	if (sizeof(struct xhci_trb) != 16)
	{
		printk("XHCI: Wrong size of struct xhci_trb\n");
		return -1;
	}

	if (msi_capable == FALSE && msix_capable == FALSE) 
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
	cntrl->doorbell = (union xhci_doorbell_register*) (cntrl->mmio_addr + (cntrl->cap->dboff));// & ~0x3));

	// Получение свойств
	cntrl->slots = cntrl->cap->hcsparams1 & 0xFF;
	cntrl->ports_num = cntrl->cap->hcsparams1 >> 24;
	cntrl->interrupters = (cntrl->cap->hcsparams1 >> 8) & 0b11111111111;
	cntrl->context_size = (cntrl->cap->hcsparams1 >> 2) & 0b1;
	printk("XHCI: caplen: %i version: %i, slots: %i, ports: %i, ints: %i, csz: %i\n",
		(cntrl->cap->caplen), (cntrl->cap->version) & 0xFFFF, cntrl->slots, cntrl->ports_num, cntrl->interrupters, cntrl->context_size);

	// Выделить память под массивы структур
	size_t ports_mem_bytes = cntrl->ports_num * sizeof(struct xhci_port_desc);
	cntrl->ports = kmalloc(ports_mem_bytes);
	memset(cntrl->ports, 0, ports_mem_bytes);

	size_t devices_mem_bytes = (cntrl->slots * sizeof(struct xhci_device*)); 
	cntrl->devices_by_slots = kmalloc(devices_mem_bytes);
	memset(cntrl->devices_by_slots, 0, devices_mem_bytes);

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
    if (rc != 0)
	{
		rc = pci_device_set_msix_vector(dev, irq);

		if (rc != 0) {
			printk("XHCI: Error assigning MSI IRQ vector: %i\n", rc);
			return -1;
		}
    }
	register_irq_handler(irq, xhci_int_hander, cntrl);

	xhci_controller_init_scratchpad(cntrl);
	xhci_controller_init_interrupts(cntrl, cntrl->event_ring);
	
	if ((rc = xhci_controller_start(cntrl)) != 0)
	{
		printk("XHCI: Error on start: %i\n", rc);
		// TODO: Clear everything
		return -1;
	}
	// Сохранить указатель на структуру
	dev->dev_data = cntrl;

	// Отправляем тестовую команду
	/*printk("Sending test command!!!\n");
	struct xhci_trb test_trb = {0,};
	test_trb.interrupt_on_completion = 1;
	test_trb.type = XHCI_TRB_NO_OP_CMD;
	xhci_command_enqueue(cntrl->cmdring, &test_trb);
	cntrl->doorbell[0].doorbell = 0;*/

	/*uint64_t flag = (1 << 3);
	while ((controller->op->crcr & flag) == 0)
	{
		printk("XHCI: CRCR %s\n", ulltoa(controller->op->crcr, 16));
	}*/

	// Процесс и поток с обраюотчиком событий
	struct process* xhci_process = create_new_process(NULL);
	process_set_name(xhci_process, "xhci");
    struct thread* xhci_event_thr = create_kthread(xhci_process, xhci_controller_event_thread_routine, cntrl);
    scheduler_add_thread(xhci_event_thr);

    return 0;
}

int xhci_controller_enqueue_cmd_wait(struct xhci_controller* controller, struct xhci_command_ring *ring, struct xhci_trb* trb, struct xhci_trb* result)
{
	size_t index = xhci_command_enqueue(ring, trb);

	struct xhci_trb* completion_trb = &ring->completions[index];
	memset(completion_trb, 0, sizeof(struct xhci_trb));

	controller->doorbell[0].doorbell = XHCI_DOORBELL_COMMAND_RING;

	while (completion_trb->raw.status == 0)
	{
		// wait
	}

	if (completion_trb->cmd_completion.completion_code != 1)
	{
		return completion_trb->cmd_completion.completion_code;
	}

	memcpy(result, completion_trb, sizeof(struct xhci_trb));

	return 0;
}

void xhci_controller_event_thread_routine(struct xhci_controller* controller)
{
	struct xhci_interrupter* interrupter = &controller->runtime->interrupters[0];
	int rc;
	while (1)
	{
		if (controller->port_status_changed == 1) 
		{
			for (uint8_t port_id = 0; port_id < controller->ports_num; port_id ++)
			{
				struct xhci_port_desc* port = &controller->ports[port_id];

				if (port->status_changed == 1)
				{
					struct xhci_port_regs *port_regs = &controller->ports_regs[port_id];
					if ((port_regs->status & XHCI_PORTSC_CSC) == XHCI_PORTSC_CSC)
					{	
						if ((port_regs->status & XHCI_PORTSC_CCS) == XHCI_PORTSC_CCS)
						{
							printk("XHCI: new port (%i) connection: %i\n", port_id, port_regs->status & XHCI_PORTSC_CCS);
							rc = xhci_controller_reset_port(controller, port_id);
							hpet_sleep(100);
							if (rc == TRUE)
							{
								printk("XHCI: Port reset successful\n");
								// порт успешно сбошен, можно инициализировать устройство
								xhci_controller_init_device(controller, port_id);
							} else
							{
								printk("XHCI: Port reset failed\n");
							}
						} 
						else
						{
							printk("XHCI: port (%i) disconnection: %i\n", port_id, port_regs->status & XHCI_PORTSC_CCS);
							
							// Получить указатель на устройство в порту
							struct xhci_device* device = controller->ports[port_id].bound_device;
							
							if (device)
							{
								uint8_t slot = device->slot_id;
								controller->devices_by_slots[slot] = NULL;
								controller->ports[port_id].bound_device = NULL;
								unregister_device(device->composite_dev);
								xhci_controller_disable_slot(controller, slot);
								xhci_free_device(device);
							}

							rc = xhci_controller_reset_port(controller, port_id);
						}
					}

					port->status_changed = 0;
				}
			}

			controller->port_status_changed = 0;
		}
	}
}

int xhci_controller_poweron(struct xhci_controller* controller, uint8_t port_id)
{
	struct xhci_port_regs *port_regs = &controller->ports_regs[port_id];

	if ((port_regs->status & XHCI_PORTSC_PORTPOWER) == XHCI_PORTSC_PORTPOWER)
	{
		return TRUE;
	}

	port_regs->status = XHCI_PORTSC_PORTPOWER;
	hpet_sleep(20);

	if (!(port_regs->status & XHCI_PORTSC_PORTPOWER))
	{
		return FALSE;
	} 
	else
	{
		return TRUE;
	}
}

int xhci_controller_reset_port(struct xhci_controller* controller, uint8_t port_id)
{
	struct xhci_port_desc *port = &controller->ports[port_id];
	struct xhci_port_regs *port_regs = &controller->ports_regs[port_id];

	int is_usb3 = port->revision_major == 3;

	if (xhci_controller_poweron(controller, port_id) == FALSE)
	{
		return FALSE;
	}

	// Очистить флаги статуса
	port_regs->status |= (XHCI_PORTSC_CSC | XHCI_PORTSC_PEC | XHCI_PORTSC_PRC);

	// Для USB 3.0 можно использовать новомодный Warm Port Reset
	uint32_t status_reset_flag = is_usb3 ? XHCI_PORTSC_WPR : XHCI_PORTSC_PORTRESET;
	uint32_t status_reset_expect_flag = is_usb3 ? XHCI_PORTSC_WRC : XHCI_PORTSC_PRC;
	// Сброс порта
	port_regs->status |= status_reset_flag;

	uint16_t tries = 100;
	while ((tries > 0) && ((port_regs->status & status_reset_expect_flag) == 0))
	{
		tries--;
		hpet_sleep(1);
	}
	if (tries == 0)
	{
		return FALSE;
	}

	hpet_sleep(3);

	// Очистить флаги статуса
	uint32_t pstatus = port_regs->status;
	pstatus |= (XHCI_PORTSC_PRC | XHCI_PORTSC_WRC | XHCI_PORTSC_CSC | XHCI_PORTSC_PEC);
	// TODO: нужно ли?
	pstatus &= (~XHCI_POSRTSC_PED);
	port_regs->status = pstatus;

	hpet_sleep(3);

	// This case could happen when the port has been reset after
    // a device disconnect event, and no device has connected since.
	if ((port_regs->status & XHCI_POSRTSC_PED) == 0)
	{
		return FALSE;
	}

	return TRUE;
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

	xhci_interrupter_upd_erdp(interrupter, event_ring);
	//uintptr_t trb_offset = event_ring->trbs_phys + event_ring->next_trb_index * sizeof(struct xhci_trb);
	//interrupter->erdp = trb_offset | XHCI_EVENT_HANDLER_BUSY;

	// Включить прерывания на этом interrupter
	interrupter->iman = interrupter->iman | XHCI_IMAN_INTERRUPT_PENDING | XHCI_IMAN_INTERRUPT_ENABLE;

	// Сбросить флаг прерывания (если было необработанное прерывание)
	controller->op->usbsts |= XHCI_STS_EINTERRUPT;
}

void xhci_controller_process_event(struct xhci_controller* controller, struct xhci_trb* event)
{
	int rc = 0;
	switch (event->type)
	{
		case XHCI_TRB_PORT_STATUS_CHANGE_EVENT:
			// По умолчанию для xhci номер порта начинается с 1
			uint8_t port_id = event->port_status_change.port_id - 1;
			printk("XHCI: port (%i) status change event\n", port_id);
			controller->ports[port_id].status_changed = 1;
			controller->port_status_changed = 1;
			break;
		case XHCI_TRB_CMD_COMPLETION_EVENT:
			size_t cmd_trb_index = (event->cmd_completion.cmd_trb_ptr - controller->cmdring->trbs_phys) / sizeof(struct xhci_trb);
#ifdef XHCI_LOG_CMD_COMPLETION
			printk("XHCI: command (%i) completed on slot (%i)!\n", cmd_trb_index, event->cmd_completion.slot_id);
#endif
			memcpy(&controller->cmdring->completions[cmd_trb_index], event, sizeof(struct xhci_trb));

			break;
		case XHCI_TRB_TRANSFER_EVENT:
			uint32_t slot_id = event->transfer_event.slot_id;
			struct xhci_device* device = controller->devices_by_slots[slot_id - 1];
			if (device != NULL)
			{
				xhci_device_handle_transfer_event(device, event);
			}

			break;
		default:
			printk("XHCI: unimplemented event %i\n", event->type);
	}
}

uint16_t xhci_get_device_max_initial_packet_size(uint8_t port_speed)
{
    uint16_t initial_max_packet_size = 0;

    switch (port_speed) 
	{
		case XHCI_USB_SPEED_LOW_SPEED: 
			initial_max_packet_size = 8;
			break;
		case XHCI_USB_SPEED_FULL_SPEED:
		case XHCI_USB_SPEED_HIGH_SPEED: 
			initial_max_packet_size = 64;
			break;
		case XHCI_USB_SPEED_SUPER_SPEED:
		case XHCI_USB_SPEED_SUPER_SPEED_PLUS:
		default: 
			initial_max_packet_size = 512;
			break;
    }

    return initial_max_packet_size;
}

int xhci_controller_init_device(struct xhci_controller* controller, uint8_t port_id)
{
	struct xhci_port_regs *port_regs = &controller->ports_regs[port_id];

	uint8_t port_speed = (port_regs->status >> 10) & 0b1111;
	printk("XHCI: port speed: %s\n", xhci_speed_string[port_speed]);

	uint16_t max_initial_packet_size = xhci_get_device_max_initial_packet_size(port_speed);

	// Попросить контроллер выделить слот (1 - ...)
	uint8_t slot = xhci_controller_alloc_slot(controller);
	if (slot == 0 || slot > controller->slots)
	{
		printk("XHCI: xhci_controller_alloc_slot() returned invalid slot (%i)\n", slot);
		return -1;
	}
	printk("XHCI: allocated slot (%i)\n", slot);

	// Создать объект устройства
	struct xhci_device* device = new_xhci_device(controller, port_id, slot);
	device->port_speed = port_speed;
	
	// Сохранить указатель в массиве по номеру слота (0 - ...)
	controller->devices_by_slots[slot - 1] = device;
	
	// Записать устройство в порт
	controller->ports[port_id].bound_device = device;
	
	// Инициализировать контексты
	int rc = xhci_device_init_contexts(device);

	// Настройка контекстов с базовым размером пакета
	xhci_device_configure_control_endpoint_ctx(device, max_initial_packet_size);

	// Записать адрес на контекст output в DCBAA
	controller->dcbaa[slot] = device->device_ctx_phys;

	// Сначала выполняем с BSR = 1. Это может быть необходимо для некоторых старых устройств
	rc = xhci_controller_address_device(controller, device->input_ctx_phys, slot, TRUE);
	if (rc != 0)
	{
		printk("XHCI: device address error (%i)!\n", rc);	
		return -1;
	}

	struct usb_device_descriptor device_descriptor;
	memset(&device_descriptor, 0, sizeof(struct usb_device_descriptor));

	// Запросим первые 8 байт дескриптора устройства
	rc = xhci_device_get_descriptor(device, &device_descriptor, 8);
	if (rc != 0) 
	{
		printk("XHCI: device descriptor request error (%i)!\n", rc);	
		return -1;
	}

	printk("XHCI: header type %i, size %i, actual packet size: %i\n", 
		device_descriptor.header.bDescriptorType, device_descriptor.header.bLength, device_descriptor.bMaxPacketSize0);
	
	// Теперь получим настоящий максимальный размер пакета
	uint16_t actual_max_packet_size = device_descriptor.bMaxPacketSize0;  
	
	// Вроде как, если устройство USB3.0, то оно отдаст значение в виде 2^x
	if (port_speed == XHCI_USB_SPEED_SUPER_SPEED)
	{
		actual_max_packet_size = (1 << device_descriptor.bMaxPacketSize0);
	}
	
	// Сравнить реальный размер пакета с тем, что расчитали по типу соединения
	if (actual_max_packet_size != max_initial_packet_size)
	{
		printk("XHCI: Actual max packet size (%i) differs (%i). updating packet size\n", actual_max_packet_size, max_initial_packet_size);
		// Обновим параметр в Endpoint Context
		rc = xhci_device_update_actual_max_packet_size(device, actual_max_packet_size);
		if (rc != 0) 
		{
			printk("XHCI: error during updating device ctrl endpoint packet size (%i)!\n", rc);	
			return -1;
		}
	}

	// Еще раз выполним, но уже без BSR
	rc = xhci_controller_address_device(controller, device->input_ctx_phys, slot, FALSE);
	if (rc != 0)
	{
		printk("XHCI: device address 2 error (%i)!\n", rc);	
		return -1;
	}

	// Еще раз запросим дескриптор, но уже в полном объеме
	rc = xhci_device_get_descriptor(device, &device_descriptor, device_descriptor.header.bLength);
	if (rc != 0) 
	{
		printk("XHCI: device descriptor 2 request error (%i)!\n", rc);	
		return -1;
	}

	printk("XHCI: Vendor: 0x%x idProduct: 0x%x bcdDevice: 0x%x\n", device_descriptor.idVendor, device_descriptor.idProduct, device_descriptor.bcdDevice);

	// Создаем общий объект устройства в ядре, не зависящий от типа контроллера 
	struct usb_device* usb_device = new_usb_device(&device_descriptor, device);
	usb_device->slot_id = slot;
	usb_device->send_request = xhci_drv_device_send_usb_request;
	usb_device->configure_endpoint = xhci_drv_device_configure_endpoint;
	usb_device->bulk_msg = xhci_drv_device_bulk_msg;
	
	// Получение информации о названии устройства
	xhci_device_get_product_strings(device, usb_device);
	printk("XHCI: Product: %s Man: %s Serial: %s\n", usb_device->product, usb_device->manufacturer, usb_device->serial);

	// Создать объект устройства
	struct device* composite_dev = new_device();
	device_set_name(composite_dev, usb_device->product);
	composite_dev->dev_type = DEVICE_TYPE_USB_COMPOSITE;
	composite_dev->dev_bus = DEVICE_BUS_USB;
	composite_dev->usb_info.usb_device = usb_device;
	//composite_dev->dev_parent = 
	
	device->usb_device = usb_device;
	device->composite_dev = composite_dev;

	// Обработка всех конфигураций
	for (uint8_t i = 0; i < device_descriptor.bNumConfigurations; i ++)
	{
		rc = xhci_device_process_configuration(device, i);
	}

	rc = register_device(composite_dev);

	return 0;
}

uint8_t xhci_controller_alloc_slot(struct xhci_controller* controller)
{
	struct xhci_trb enable_slot_trb;
	memset(&enable_slot_trb, 0, sizeof(struct xhci_trb));
	enable_slot_trb.type = XHCI_TRB_ENABLE_SLOT_CMD;

	struct xhci_trb result;
	xhci_controller_enqueue_cmd_wait(controller, controller->cmdring, &enable_slot_trb, &result);

	return result.cmd_completion.slot_id;
}

int xhci_controller_disable_slot(struct xhci_controller* controller, uint8_t slot)
{
	struct xhci_trb disable_slot_trb;
	memset(&disable_slot_trb, 0, sizeof(struct xhci_trb));
	disable_slot_trb.type = XHCI_TRB_DISABLE_SLOT_CMD;
	disable_slot_trb.disable_slot_command.slot_id = slot;

	struct xhci_trb result;
	return xhci_controller_enqueue_cmd_wait(controller, controller->cmdring, &disable_slot_trb, &result);
}

int xhci_controller_address_device(struct xhci_controller* controller, uintptr_t address, uint8_t slot_id, int bsr)
{
	struct xhci_trb address_device_trb;
	memset(&address_device_trb, 0, sizeof(struct xhci_trb));
	address_device_trb.type = XHCI_TRB_ADDRESS_DEVICE_CMD;
	address_device_trb.address_device.input_context_pointer = address;
	address_device_trb.address_device.slot_id = slot_id;
	address_device_trb.address_device.block_set_address_request = (bsr == TRUE) ? 1 : 0;

	struct xhci_trb result;
	return xhci_controller_enqueue_cmd_wait(controller, controller->cmdring, &address_device_trb, &result);
}

int xhci_controller_device_eval_ctx(struct xhci_controller* controller, uintptr_t address, uint8_t slot)
{
	struct xhci_trb address_device_trb;
	memset(&address_device_trb, 0, sizeof(struct xhci_trb));
	address_device_trb.type = XHCI_TRB_EVALUATE_CONTEXT_CMD;
	// Можем использовать структуру от address device
	address_device_trb.address_device.input_context_pointer = address;
	address_device_trb.address_device.slot_id = slot;
	address_device_trb.address_device.block_set_address_request = 0;

	struct xhci_trb result;
	return xhci_controller_enqueue_cmd_wait(controller, controller->cmdring, &address_device_trb, &result);
}

int xhci_controller_configure_endpoint(struct xhci_controller* controller, uint8_t slot, uintptr_t address, uint8_t deconfigure)
{
	struct xhci_trb configure_ep_trb;
	memset(&configure_ep_trb, 0, sizeof(struct xhci_trb));
	configure_ep_trb.type = XHCI_TRB_CONFIGURE_ENDPOINT_CMD;

	configure_ep_trb.configure_endpoint_command.slot_id = slot;
	configure_ep_trb.configure_endpoint_command.input_context_pointer = address;
	configure_ep_trb.configure_endpoint_command.deconfigure = deconfigure;

	struct xhci_trb result;
	return xhci_controller_enqueue_cmd_wait(controller, controller->cmdring, &configure_ep_trb, &result);
}

struct xhci_command_ring *xhci_create_command_ring(size_t ntrbs)
{
	struct xhci_command_ring *cmdring = kmalloc(sizeof(struct xhci_command_ring));
	cmdring->trbs_num = ntrbs;
	cmdring->cycle_bit = XHCI_CR_CYCLE_STATE;
	cmdring->next_trb_index = 0;

	// Выделить память под TRB
	size_t cmd_ring_pages = 0;
	size_t cmd_ring_sz = ntrbs * sizeof(struct xhci_trb);
	cmdring->trbs_phys = (uintptr_t) pmm_alloc(cmd_ring_sz, &cmd_ring_pages);
	cmdring->trbs = (struct xhci_trb*) map_io_region(cmdring->trbs_phys, cmd_ring_sz);
	memset(cmdring->trbs, 0, cmd_ring_sz);

	// Выделить память под completions
	cmdring->completions = kmalloc(cmd_ring_sz);
	memset(cmdring->completions, 0, cmd_ring_sz);

	// Установить последнее TRB как ссылку на первое
	cmdring->trbs[ntrbs - 1].type = XHCI_TRB_TYPE_LINK;
	cmdring->trbs[ntrbs - 1].link.cycle_bit = cmdring->cycle_bit;
	cmdring->trbs[ntrbs - 1].link.cycle_enable = 1;
	cmdring->trbs[ntrbs - 1].link.address = cmdring->trbs_phys;

	return cmdring;
}

size_t xhci_command_enqueue(struct xhci_command_ring *ring, struct xhci_trb* trb)
{
	// Запишем Cycle State (чтобы на следующем круге XHCI видел это как невыполненное)
	trb->cycle_bit = ring->cycle_bit;

	size_t tmp_index = ring->next_trb_index;
	memcpy(&ring->trbs[ring->next_trb_index], trb, sizeof(struct xhci_trb));

	if (++ring->next_trb_index == ring->trbs_num - 1)
	{
		// Достигли конечной TRB
		ring->trbs[ring->trbs_num - 1].link.cycle_bit = ring->cycle_bit;
		// Сбрасываем указатель на 0
		ring->next_trb_index = 0;
		// Инвертируем Cycle State
		ring->cycle_bit = !ring->cycle_bit;
	}

	return tmp_index;
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

	return eventring;
}

int xhci_event_ring_deque(struct xhci_event_ring *ring, struct xhci_trb *trb)
{
	struct xhci_trb* dequed = &ring->trbs[ring->next_trb_index];
	if (dequed->cycle_bit != ring->cycle_bit)
	{
		return 0;
	}

	memcpy(trb, dequed, sizeof(struct xhci_trb));

	// Увеличить позицию
	ring->next_trb_index++;

	// Если считали полный круг - возвращаемся в начало
	if (ring->next_trb_index >= ring->trb_count)
	{
		ring->next_trb_index = 0;
		ring->cycle_bit = !ring->cycle_bit;
	}
}

struct xhci_transfer_ring *xhci_create_transfer_ring(size_t ntrbs)
{
	struct xhci_transfer_ring *transfer_ring = kmalloc(sizeof(struct xhci_transfer_ring));
	transfer_ring->trb_count = ntrbs;
	transfer_ring->cycle_bit = XHCI_CR_CYCLE_STATE;
	transfer_ring->enqueue_ptr = 0;
	transfer_ring->dequeue_ptr = 0;
	transfer_ring->compl = NULL;

	size_t transfer_ring_buffer_size = ntrbs * sizeof(struct xhci_trb);
	// Выделить память
	transfer_ring->trbs_phys = (uintptr_t) pmm_alloc(transfer_ring_buffer_size, &transfer_ring->trbs_numpages);
	transfer_ring->trbs = map_io_region(transfer_ring->trbs_phys, transfer_ring_buffer_size);
	memset(transfer_ring->trbs, 0, transfer_ring_buffer_size);

	// Установить последнее TRB как ссылку на первое
	transfer_ring->trbs[ntrbs - 1].type = XHCI_TRB_TYPE_LINK;
	transfer_ring->trbs[ntrbs - 1].link.cycle_bit = transfer_ring->cycle_bit;
	transfer_ring->trbs[ntrbs - 1].link.cycle_enable = 1;
	transfer_ring->trbs[ntrbs - 1].link.address = transfer_ring->trbs_phys;

	return transfer_ring;
}

void xhci_transfer_ring_create_compl_table(struct xhci_transfer_ring *ring)
{
	size_t compl_sz = sizeof(struct xhci_transfer_ring_completion) * ring->trb_count;
	ring->compl = kmalloc(compl_sz);
	memset(ring->compl, 0, compl_sz);
}

void xhci_free_transfer_ring(struct xhci_transfer_ring *ring)
{
	unmap_io_region(ring->trbs, ring->trbs_numpages * PAGE_SIZE);
	pmm_free_pages(ring->trbs_phys, ring->trbs_numpages);
	KFREE_SAFE(ring->compl);
	kfree(ring);
}

size_t xhci_transfer_ring_enqueue(struct xhci_transfer_ring *ring, struct xhci_trb* trb)
{
	// Запишем Cycle State (чтобы на следующем круге XHCI видел это как невыполненное)
	trb->cycle_bit = ring->cycle_bit;

	size_t tmp_index = ring->enqueue_ptr;
	memcpy(&ring->trbs[tmp_index], trb, sizeof(struct xhci_trb));

	if (++ring->enqueue_ptr == ring->trb_count - 1)
	{
		// Достигли конечной TRB
		ring->trbs[ring->trb_count - 1].link.cycle_bit = ring->cycle_bit;
		// Сбрасываем указатель на 0
		ring->enqueue_ptr = 0;
		// Инвертируем Cycle State
		ring->cycle_bit = !ring->cycle_bit;
	}

	return tmp_index;
}

uintptr_t xhci_transfer_ring_get_cur_phys_ptr(struct xhci_transfer_ring *ring)
{
	return ring->trbs_phys + ring->enqueue_ptr * sizeof(struct xhci_trb);
}

void xhci_interrupter_upd_erdp(struct xhci_interrupter *intr, struct xhci_event_ring *ring)
{
	uintptr_t trb_offset = ring->trbs_phys + ring->next_trb_index * sizeof(struct xhci_trb);
	intr->erdp = trb_offset | XHCI_EVENT_HANDLER_BUSY;
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

			//printk("XHCI: compatible port count %i\n", prot_cap->compatible_port_count);
			for (size_t i = 0; i < prot_cap->compatible_port_count; i++)
			{
				uint8_t port_id = prot_cap->compatible_port_offset + i - 1;
				struct xhci_port_desc* port = &controller->ports[port_id];
				port->proto_cap = prot_cap;
				port->revision_major = prot_cap->major_revision;
				port->revision_minor = prot_cap->minor_revision;
				//printk("XHCI: port %i, version (%i.%i)\n", port_id, port->revision_major, port->revision_minor);
			}
		} else if (cap->capability_id == XHCI_EXT_CAP_LEGACY_SUPPORT)
		{
			//printk("XHCI: legacy support capability\n");

			struct xhci_legacy_support_cap* legacy_cap = (struct xhci_legacy_support_cap*) ext_cap_ptr;
			
			uint32_t mask = (XHCI_LEGACY_SMI_ENABLE | 
							XHCI_LEGACY_SMI_ON_OS_OWNERSHIP | 
							XHCI_LEGACY_SMI_ON_HOST_ERROR |
							XHCI_LEGACY_SMI_ON_PCI_COMMAND |
				 			XHCI_LEGACY_SMI_ON_BAR);

			// Выключить все SMI
			//legacy_cap->usblegctlsts &= ~mask;
			//hpet_sleep(10);

			// Если BIOS владеет устройством, значит надо его отобрать у него
			if (legacy_cap->bios_owned_semaphore) 
			{
				printk("XHCI: Performing handoff\n");
				legacy_cap->os_owned_semaphore = 1;
				hpet_sleep(10);
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
	struct xhci_interrupter* interrupter = &data->runtime->interrupters[0];
	volatile uint32_t usbsts = data->op->usbsts;
	
	if ((usbsts & XHCI_STS_HALT) == XHCI_STS_HALT)
	{
		printk("XHCI: Host Controller Halted\n");
	}

	if ((usbsts & XHCI_STS_HSE) == XHCI_STS_HSE)
	{
		printk("XHCI: Host System Error\n");
	}

	if ((usbsts & XHCI_STS_HCE) == XHCI_STS_HCE)
	{
		printk("XHCI: Host Controller Error\n");
	}

	if ((usbsts & XHCI_STS_EINTERRUPT) == XHCI_STS_EINTERRUPT)
	{
		//printk("XHCI: Event Interrupt\n");

		struct xhci_trb event;
		while (xhci_event_ring_deque(data->event_ring, &event))
		{
			xhci_controller_process_event(data, &event);
		}
		// ??? может это надо делать внутри цикла ???
		xhci_interrupter_upd_erdp(interrupter, data->event_ring);
	}

	// IMAN - acknowledge
	interrupter->iman = interrupter->iman | XHCI_IMAN_INTERRUPT_PENDING;

	// Очистить флаг прерывания в статусе
	data->op->usbsts = usbsts;
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
