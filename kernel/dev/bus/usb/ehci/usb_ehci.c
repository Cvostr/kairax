#include "usb_ehci.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "dev/device_man.h"
#include "mem/iomem.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "string.h"
#include "kairax/stdio.h"
#include "io.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "kairax/kstdlib.h"

int ehci_allocate_pool(struct ehci_controller *ehc, size_t qh_pool, size_t td_pool)
{
	size_t QH_SIZE = align(sizeof(struct ehci_qh), 32);
	size_t TD_SIZE = align(sizeof(struct ehci_td), 32);

	size_t QH_buffersz = QH_SIZE * qh_pool;
	size_t TD_buffersz = TD_SIZE * td_pool;

	//printk("EHCI: QH size %i TD size %i\n", QH_SIZE, TD_SIZE);

	size_t qh_pages;
	void *qh_pool_phys = pmm_alloc(QH_buffersz, &qh_pages);
	// Адрес должен быть выделен и быть не больше 4Гб
	if (qh_pool_phys == NULL || qh_pool_phys > UINT32_MAX)
	{
		printk("EHCI: QH pool allocation error!\n");
		return -ENOMEM;
	}

	size_t td_pages;
	void *td_pool_phys = pmm_alloc(QH_buffersz, &td_pages);
	// Адрес должен быть выделен и быть не больше 4Гб
	if (td_pool_phys == NULL || td_pool_phys > UINT32_MAX)
	{
		printk("EHCI: TD pool allocation error!\n");
		pmm_free_pages(qh_pool_phys, qh_pages);
		return -ENOMEM;
	}

	// Замаппим и занулим память для QH
	ehc->qh_pool = map_io_region(qh_pool_phys, qh_pages * PAGE_SIZE);
	memset(ehc->qh_pool, 0, qh_pages * PAGE_SIZE);

	// Замаппим и занулим память для TD
	ehc->td_pool = map_io_region(td_pool_phys, td_pages * PAGE_SIZE);
	memset(ehc->td_pool, 0, td_pages * PAGE_SIZE);

	ehc->qh_pool_size = qh_pool;
	ehc->td_pool_size = td_pool;

	return 0;
}

struct ehci_qh *ehci_alloc_qh(struct ehci_controller *ehc)
{
	size_t QH_SIZE = align(sizeof(struct ehci_qh), 32);

	struct ehci_qh *result = NULL;
	struct ehci_qh *current = NULL;

	for (size_t i = 0; i < ehc->qh_pool_size; i ++)
	{
		current = ehc->qh_pool + QH_SIZE * i;
		if (current->acquired == FALSE)
		{
			current->acquired = TRUE;
			result = current;
			break;
		}
	}

	return result;
}

void ehci_free_qh(struct ehci_controller *ehc, struct ehci_qh *qh)
{
	qh->acquired = FALSE;
}

struct ehci_td *ehci_alloc_td(struct ehci_controller *ehc)
{
	size_t TD_SIZE = align(sizeof(struct ehci_td), 32);

	struct ehci_td *result = NULL;
	struct ehci_td *current = NULL;

	for (size_t i = 0; i < ehc->td_pool_size; i ++)
	{
		current = ehc->td_pool + TD_SIZE * i;
		if (current->acquired == FALSE)
		{
			current->acquired = TRUE;
			result = current;
			break;
		}
	}
	
	return result;
}

void ehci_free_td(struct ehci_controller *ehc, struct ehci_td *td)
{
	td->acquired = FALSE;
}

uint8_t ehci_next_device_address(struct ehci_controller *ehc)
{
	// TODO: что то мне подсказывает, что тут возможны коллизии
	// Надо учитывать уже выделенные адреса
	ehc->device_address_counter ++;
	// В EHCI QH под этот номер отведено 7 бит
	if (ehc->device_address_counter >= 127)
		ehc->device_address_counter = 1;

	return ehc->device_address_counter;
}

// EHCI Revision 1.0
// 4.8.1
void ehci_qh_link_qh(struct ehci_qh *qh_parent, struct ehci_qh *qh)
{
	// Сначала нам надо установить для вставляемой QH адрес следующей QH
	// Он будет равен QHLINK от qh_parent
	qh->link_ptr.raw = qh_parent->link_ptr.raw;
	// Заполним указатели на следующий QH и предыдущий
	qh->prev_ptr = qh_parent;
	qh->next_ptr = qh_parent->next_ptr;
	// Установим новый QHLP для qh_parent
	uintptr_t qh_phys = vmm_get_physical_address(qh);
	qh_parent->link_ptr.type = EHCI_TYPE_QH;
	qh_parent->link_ptr.lp = qh_phys >> 5;
	qh_parent->link_ptr.terminate = 0;
}

void ehci_qh_link_td(struct ehci_qh *qh, struct ehci_td *td)
{
	uintptr_t td_phys = vmm_get_physical_address(td);
	qh->next.terminate = 0;
	qh->next.lp = td_phys >> 5;
}

void ehci_td_link_td(struct ehci_td *td_parent, struct ehci_td *td)
{
	uintptr_t td_phys = vmm_get_physical_address(td);
	td_parent->next.terminate = 0;
	td_parent->next.lp = td_phys >> 5;
}

void ehci_enqueue_qh(struct ehci_controller* hci, struct ehci_qh *qh)
{
	// TODO: улучшить
	ehci_qh_link_qh(hci->async_head, qh);
	//hci->async_tail = qh;
}

// EHCI Revision 1.0
// 4.8.2
void ehci_dequeue_qh(struct ehci_controller* hci, struct ehci_qh *qh)
{
	// TODO: улучшить
	struct ehci_qh *prev = qh->prev_ptr;
	struct ehci_qh *next = qh->next_ptr;

	prev->link_ptr.raw = qh->link_ptr.raw;

	// Уведомим устройство о том, что что то удалили из очереди 
	uint32_t usbcmd = ehci_op_read32(hci, EHCI_REG_OP_USBCMD);
	ehci_op_write32(hci, EHCI_REG_OP_USBCMD, usbcmd | EHCI_USBCMD_INT_ON_AAB);

	// надо бы дождаться USBSTS.Interrupt on Async Advance
	// TODO: сделать через прерывание
	uint32_t usbsts;
	while (((usbsts = ehci_op_read32(hci, EHCI_REG_OP_USBSTS)) & EHCI_USBSTS_INT_ON_ASYNC_ADV) == 0)
	{
		
	}

	ehci_op_write32(hci, EHCI_REG_OP_USBSTS, usbsts);
}

void ehci_td_set_virtual_databuffers(struct ehci_td *td, uintptr_t addr, int has64)
{
	uintptr_t ptr = addr;
	uintptr_t page = vmm_get_physical_address(ptr);

	// Первую страницу записываем со смещением, потому что оно там поддерживается
	td->buffer_ptr_list[0] = (uint32_t)(page);
	if (has64)
		td->ext_buffer_ptr[0] = (uint32_t) (page >> 32);

	// надо выровнять, потому что остальные указатели в TD это не поддерживают
	// В данном случае выравнивание вниз, потому что потом в цикле размер страницы все равно прибавим
	ptr &= (~0xFFF);

	for (int i = 1; i < 4; i ++)
	{
		ptr += 0x1000;
		page = vmm_get_physical_address(ptr);
	
		td->buffer_ptr_list[i] = (uint32_t)(page);
		if (has64)
			td->ext_buffer_ptr[i] = (uint32_t) (page >> 32);
	}
}

void ehci_td_set_databuffers(struct ehci_td *td, uintptr_t addr, int has64)
{
	uintptr_t ptr = addr;
	// Первую страницу записываем со смещением, потому что оно там поддерживается
	td->buffer_ptr_list[0] = (uint32_t)(ptr);
	if (has64)
		td->ext_buffer_ptr[0] = (uint32_t) (ptr >> 32);

	// надо выровнять, потому что остальные указатели в TD это не поддерживают
	// В данном случае выравнивание вниз, потому что потом в цикле размер страницы все равно прибавим
	ptr &= (~0xFFF);

	for (int i = 1; i < 4; i ++)
	{
		ptr += 0x1000;
	
		td->buffer_ptr_list[i] = (uint32_t)(ptr);
		if (has64)
			td->ext_buffer_ptr[i] = (uint32_t) (ptr >> 32);
	}
}

// Операции с регистрами
void ehci_write32(struct ehci_controller* cntrl, off_t offset, uint32_t value)
{
	if (cntrl->bar_type == BAR_TYPE_IO)
	{
		outl(cntrl->mmio_addr_phys + offset, value);
	} 
	else
	{
		volatile uint32_t* addr = (cntrl->mmio_addr + offset);
		*addr = value;
	}
}

void ehci_op_write32(struct ehci_controller* cntrl, off_t offset, uint32_t value)
{
	ehci_write32(cntrl, cntrl->op_offset + offset, value);
}

uint8_t ehci_read8(struct ehci_controller* cntrl, off_t offset)
{
	if (cntrl->bar_type == BAR_TYPE_IO)
	{
		return inb(cntrl->mmio_addr_phys + offset);
	} 

	volatile uint8_t* addr = (cntrl->mmio_addr + offset);
	return *addr;
}

uint16_t ehci_read16(struct ehci_controller* cntrl, off_t offset)
{
	if (cntrl->bar_type == BAR_TYPE_IO)
	{
		return inw(cntrl->mmio_addr_phys + offset);
	} 

	volatile uint16_t* addr = (cntrl->mmio_addr + offset);
	return *addr;
}

uint32_t ehci_read32(struct ehci_controller* cntrl, off_t offset)
{
	if (cntrl->bar_type == BAR_TYPE_IO)
	{
		return inl(cntrl->mmio_addr_phys + offset);
	} 

	volatile uint32_t* addr = (cntrl->mmio_addr + offset);
	return *addr;
}

uint32_t ehci_op_read32(struct ehci_controller* cntrl, off_t offset)
{
	return ehci_read32(cntrl, cntrl->op_offset + offset);
}

int ehci_reset(struct ehci_controller* cntrl)
{
	uint32_t usbcmd = ehci_op_read32(cntrl, EHCI_REG_OP_USBCMD);
	if ((usbcmd & EHCI_USBCMD_RS) == EHCI_USBCMD_RS)
	{
		printk("EHCI: Disabling before reset\n");
		ehci_op_write32(cntrl, EHCI_REG_OP_USBCMD, usbcmd & (~EHCI_USBCMD_RS));

		// Ожидаем, пока не будет взведен HCHalted
		while ((ehci_op_read32(cntrl, EHCI_REG_OP_USBSTS) & EHCI_USBSTS_HCHALTED) != EHCI_USBSTS_HCHALTED)
		{
			hpet_sleep(1);
		}
	}

	printk("EHCI: Performing reset\n");
	// Выполняем сброс
	ehci_op_write32(cntrl, EHCI_REG_OP_USBCMD, EHCI_USBCMD_HCRESET);

	// ждем сброса
	while (((usbcmd = ehci_op_read32(cntrl, EHCI_REG_OP_USBCMD)) & EHCI_USBCMD_HCRESET) == EHCI_USBCMD_HCRESET)
	{
		hpet_sleep(1);
	}

	if ((ehci_op_read32(cntrl, EHCI_REG_OP_USBINTR) != 0x0) ||
		(ehci_op_read32(cntrl, EHCI_REG_OP_FRINDEX) != 0x0) ||
		(ehci_op_read32(cntrl, EHCI_REG_OP_CTRLDSSEGMENT) != 0x0) ||
		(ehci_op_read32(cntrl, EHCI_REG_OP_CONFIGFLAG) != 0x0))
	{
		printk("EHCI: Register values after reset are incorrect\n");
		return -1;
	}

	return 0;
}

// Вернет 0, если RESET успешный и устройство - High Speed
int ehci_reset_port(struct ehci_controller* hci, uint32_t port_reg)
{
	uint32_t portsc;
	// сброс порта
	// Взводим бит PORT_RESET и снимаем PORT_ENABLE
	portsc = ehci_op_read32(hci, port_reg);
	ehci_op_write32(hci, port_reg, (portsc & ~EHCI_PORTSC_ENABLED) | EHCI_PORTSC_PORT_RESET);
	// ждем 50мс
	hpet_sleep(50);
	// Снимаем PORT_RESET
	portsc = ehci_op_read32(hci, port_reg);
	ehci_op_write32(hci, port_reg, portsc & (~EHCI_PORTSC_PORT_RESET));

	// ждем 100мс пока не будет взведен флаг PORT_ENABLED
	int timer = 10; //100 ms
	while (timer > 0)
	{
		hpet_sleep(10);
		portsc = ehci_op_read32(hci, port_reg);

		if ((portsc & EHCI_PORTSC_CONNECT_STATUS) == 0)
		{	
			// никакое устройство в порт не вставлено
			return 1;
		}

		if ((portsc & EHCI_PORTSC_ENABLED) == EHCI_PORTSC_ENABLED)
		{
			// Устройство вставлено и является High Speed устройством. другие EHCI и не поддерживает
			return 0;
		}
	}

	// По всей видимости это не Low или Full Speed устройство
	return 2;
}

int ehci_qh_to_error_code(struct ehci_qh *qh, int default_code)
{
	if (qh->token.status.babble == 1)
		return -EOVERFLOW;
	if (qh->token.status.missed == 1)
		return -EIO;	// -EPROTO
	if (qh->token.status.data_buffer_error == 1)
		return -EIO;	// -ENOSR (IN) or -ECOMM (OUT)
	if (qh->token.status.transaction_error == 1)
		return -EPIPE;

	return default_code;
}

int ehci_control(struct ehci_device *device, struct usb_device_request *request, void* buffer, uint32_t length)
{
	int rc = 0;
	struct ehci_controller *hci = device->hci;
	uint8_t data_toggle = 0;
	struct ehci_td *setup = NULL;
	struct ehci_td *data = NULL;
	struct ehci_td *status = NULL;
	struct ehci_td *last = NULL; 
	void* reqphys = vmm_get_physical_address(request);
	struct ehci_qh *qh = ehci_alloc_qh(hci);

	if (qh == NULL)
		return -ENOMEM;

	// Control Endpoint
	qh->ep_ch.endpt = 0;
	qh->ep_ch.device_address = device->address;
	qh->ep_ch.eps = device->speed;
	qh->ep_ch.max_pkt_length = device->controlEpMaxPacketSize;
	qh->ep_ch.dtc = 1;
	qh->ep_ch.rl = 5;
	QH_LINK_TERMINATE(qh);

	// Если это Low Speed или Full Speed устройство, то при Control передачах надо взводить флаг
	if (device->speed != EHCI_EP_SPEED_HIGH_SPEED)
	{
		qh->ep_ch.c = 1;
	}

	//qh->ep_cap.port = device->port_id;
	//qh->ep_cap.hub_addr = 0; // TODO: Hubs

	setup = ehci_alloc_td(hci);
	if (setup == NULL)
	{
		rc = -ENOMEM;
		goto exit;
	}

	// настраиваем setup
	setup->token.pid_code = EHCI_PID_CODE_SETUP;
	setup->token.total_len = sizeof(struct usb_device_request);
	setup->token.data_toggle = data_toggle;
	setup->token.err_count = 3;
	setup->token.status.active = 1;
	ehci_td_set_databuffers(setup, reqphys, hci->mode64);
	// оставляем указатель, чтобы потом прицепить status td к setup или data
	last = setup;

	if (buffer != NULL && length > 0)
	{
		data = ehci_alloc_td(hci);
		if (data == NULL)
		{
			rc = -ENOMEM;
			goto exit;
		}

		data_toggle ^= 1;

		data->token.pid_code = 
			(request->transfer_direction == USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST) ? EHCI_PID_CODE_IN : EHCI_PID_CODE_OUT;
		data->token.err_count = 3;
		data->token.status.active = 1;
		data->token.total_len = length;
		data->token.data_toggle = data_toggle;
		ehci_td_set_virtual_databuffers(data, buffer, hci->mode64);

		ehci_td_link_td(setup, data);
		last = data;
	}

	// Настраиваем Status
	status = ehci_alloc_td(hci);
	if (status == NULL)
	{
		rc = -ENOMEM;
		goto exit;
	}

	status->token.pid_code = 
		(request->transfer_direction == USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST) ? EHCI_PID_CODE_OUT : EHCI_PID_CODE_IN;
	status->token.status.active = 1;
	status->token.data_toggle = 1;
	status->token.err_count = 3;
	status->token.total_len = 0;
	status->token.ioc = 1;
	ehci_td_link_td(last, status);
	TD_LINK_TERMINATE(status)

	// Инициализировать QH с SETUP TD
	ehci_qh_link_td(qh, setup);

	// Отправить QH на выполнение
	ehci_enqueue_qh(hci, qh);

	rc = -ETIMEDOUT;
	int timeout = 1000;
	while ((timeout --) > 0)
	{
		if (qh->token.status.halted == 1)
		{
			printk("EHCI: Halted %i %i %i %i\n" , qh->token.status.missed, qh->token.status.babble, qh->token.status.transaction_error, qh->token.status.data_buffer_error);
			printk("EHCI: SETUP (halt %i) %i %i %i %i\n", setup->token.status.halted, setup->token.status.missed, setup->token.status.babble, setup->token.status.transaction_error, setup->token.status.data_buffer_error);
			if (data)
				printk("EHCI: DATA (halt %i) %i %i %i %i\n", data->token.status.halted, data->token.status.missed, data->token.status.babble, data->token.status.transaction_error, data->token.status.data_buffer_error);
			printk("EHCI: STATUS (halt %i) %i %i %i %i\n", status->token.status.halted, status->token.status.missed, status->token.status.babble, status->token.status.transaction_error, status->token.status.data_buffer_error);
			rc = ehci_qh_to_error_code(qh, -EIO);
			break;
		}
		
		int completed = qh->next.terminate == 1;

		if (qh->token.status.active == 0 && completed)
		{
			rc = ehci_qh_to_error_code(qh, 0);
			break;
		}

		hpet_sleep(1);
	}

	ehci_dequeue_qh(hci, qh);

exit:
	if (setup)
		ehci_free_td(hci, setup);
	if (data)
		ehci_free_td(hci, data);
	if (status)
		ehci_free_td(hci, status);
	ehci_free_qh(hci, qh);
	return rc;
}

int ehci_drv_device_send_usb_request(struct usb_device* dev, struct usb_device_request* req, void* out, uint32_t length)
{
    return ehci_control(dev->controller_device_data, req, out, length);
}

int ehci_drv_device_bulk_msg(struct usb_device* dev, struct usb_endpoint* endpoint, void* data, uint32_t length)
{
	int rc = 0;
	struct ehci_device *hci_device = dev->controller_device_data;
	struct ehci_controller *hci = hci_device->hci;
	uint8_t data_toggle = 0;
	struct ehci_td *td_data = NULL;

	if (data == NULL && length == 0)
	{
		return -1;
	}

	struct ehci_qh *qh = ehci_alloc_qh(hci);

	if (qh == NULL)
		return -ENOMEM;

	// Bulk Endpoint
	qh->ep_ch.endpt = endpoint->descriptor.bEndpointAddress & 0xF;
	qh->ep_ch.device_address = hci_device->address;
	qh->ep_ch.eps = hci_device->speed;
	qh->ep_ch.max_pkt_length = endpoint->descriptor.wMaxPacketSize;
	qh->ep_ch.dtc = 1;
	qh->ep_ch.rl = 5;
	QH_LINK_TERMINATE(qh);

	//qh->ep_cap.port = device->port_id;
	//qh->ep_cap.hub_addr = 0; // TODO: Hubs


	td_data = ehci_alloc_td(hci);
	if (td_data == NULL)
	{
		rc = -ENOMEM;
		goto exit;
	}

	data_toggle ^= 1;

	int pid_code = ((endpoint->descriptor.bEndpointAddress & 0x80) == 0x80) ? EHCI_PID_CODE_IN : EHCI_PID_CODE_OUT;

	td_data->token.pid_code = pid_code;
	td_data->token.err_count = 3;
	td_data->token.status.active = 1;
	td_data->token.total_len = length;
	td_data->token.data_toggle = data_toggle;
	ehci_td_set_databuffers(td_data, data, hci->mode64);
	TD_LINK_TERMINATE(td_data)

	// Инициализировать QH с SETUP TD
	ehci_qh_link_td(qh, td_data);

	// Отправить QH на выполнение
	ehci_enqueue_qh(hci, qh);

	rc = -ETIMEDOUT;
	int timeout = 1000;
	while ((timeout --) > 0)
	{
		if (qh->token.status.halted == 1)
		{
			printk("EHCI: Halted %i %i %i %i\n" , qh->token.status.missed, qh->token.status.babble, qh->token.status.transaction_error, qh->token.status.data_buffer_error);
			if (data)
				printk("EHCI: DATA (halt %i) %i %i %i %i\n", td_data->token.status.halted, td_data->token.status.missed, td_data->token.status.babble, td_data->token.status.transaction_error, td_data->token.status.data_buffer_error);

			rc = ehci_qh_to_error_code(qh, -EIO);
			break;
		}
		
		int completed = qh->next.terminate == 1;

		if (qh->token.status.active == 0 && completed)
		{
			rc = ehci_qh_to_error_code(qh, 0);
			break;
		}

		hpet_sleep(1);
	}

	ehci_dequeue_qh(hci, qh);

exit:
	ehci_free_td(hci, td_data);
	ehci_free_qh(hci, qh);
	return rc;
}

int ehci_drv_send_async_msg(struct usb_device* dev, struct usb_endpoint* endpoint, struct usb_msg *msg)
{
	return -ENODEV;
}

int ehci_drv_device_configure_endpoint(struct usb_device* dev, struct usb_endpoint* endpoint)
{
	return 0;
}

int ehci_init_device(struct ehci_controller* hci, int portnum)
{
	int rc;
	struct ehci_device *ehci_dev = kmalloc(sizeof(struct ehci_device));
	ehci_dev->hci = hci;
	ehci_dev->port_id = portnum;
	ehci_dev->speed = EHCI_EP_SPEED_HIGH_SPEED;
	// Для первых двух запросов используем 0
	ehci_dev->address = 0;
	// До того, как мы получили дескриптор устройства, считаем, что 64
	ehci_dev->controlEpMaxPacketSize = 64;

	struct usb_device_descriptor device_descriptor;
	memset(&device_descriptor, 0, sizeof(struct usb_device_descriptor));

	struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	req.bRequest = USB_DEVICE_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DESCRIPTOR_TYPE_DEVICE << 8);
	req.wIndex = 0;
	req.wLength = 8;

    rc = ehci_control(ehci_dev, &req, &device_descriptor, 8);
	if (rc != 0)
	{
		printk("EHCI: Error requesting device descriptor 1 (%i)\n", rc);
		return rc;
	}
/*
	printk("Type 0x%x\n", device_descriptor.header.bDescriptorType);
	printk("Length 0x%x\n", device_descriptor.header.bLength);
	printk("MaxEpSize 0x%x\n", device_descriptor.bMaxPacketSize0);
*/
	ehci_dev->controlEpMaxPacketSize = device_descriptor.bMaxPacketSize0;

	// Этап установки адреса для устройства
	// Снчала получим следующий адрес
	uint8_t dev_addr = ehci_next_device_address(hci);

	// Выполняем SET_ADDRESS
	struct usb_device_request set_addr_req;
	set_addr_req.type = USB_DEVICE_REQ_TYPE_STANDART;
	set_addr_req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	set_addr_req.recipient = USB_DEVICE_REQ_RECIPIENT_DEVICE;
	set_addr_req.bRequest = USB_DEVICE_REQ_SET_ADDRESS;
	set_addr_req.wValue = dev_addr;
	set_addr_req.wIndex = 0;
	set_addr_req.wLength = 0;
	rc = ehci_control(ehci_dev, &set_addr_req, NULL, 0);
	if (rc != 0)
	{
		printk("EHCI: Error during SET_ADDRESS\n");
		return rc;
	}

	// Сохраняем новый адрес
	ehci_dev->address = dev_addr;

	// теперь можем полностью считать Device Descriptor
	// будем использовать уже имеющуюся структуру, подправив только размер ответа
	req.wLength = device_descriptor.header.bLength;
    rc = ehci_control(ehci_dev, &req, &device_descriptor, device_descriptor.header.bLength);
	if (rc != 0)
	{
		printk("EHCI: Error requesting device descriptor 2\n");
		return rc;
	}

	printk("bDeviceClass 0x%x, bDeviceSubclass 0x%x\n", device_descriptor.bDeviceClass, device_descriptor.bDeviceSubClass);
	printk("idVendor 0x%x, idProduct 0x%x\n", device_descriptor.idVendor, device_descriptor.idProduct);
	printk("bNumConfigurations %i\n", device_descriptor.bNumConfigurations);

	// Создаем общий объект устройства в ядре, не зависящий от типа контроллера 
	struct usb_device* usb_device = new_usb_device(&device_descriptor, ehci_dev);
	usb_device->state = USB_STATE_ATTACHED;
	//usb_device->slot_id = slot;
	usb_device->send_request = ehci_drv_device_send_usb_request;
	usb_device->bulk_msg = ehci_drv_device_bulk_msg;
	usb_device->async_msg = ehci_drv_send_async_msg;
	usb_device->configure_endpoint = ehci_drv_device_configure_endpoint;
	/*
	usb_device->send_async_request = xhci_drv_device_send_usb_async_request;*/

	// Смотрим, что в iManufacturer, iProduct, iSerialNumber
	// Если все три значения равны 0, то считаем, что устройство вообще не поддерживает String Descriptors
	// И мы не будем даже запрашивать код языка, чтобы не ловить ошибки
	if (device_descriptor.iManufacturer != 0 && device_descriptor.iProduct != 0 && device_descriptor.iSerialNumber != 0)
	{
		struct usb_string_language_descriptor* lang_descriptor = kmalloc(sizeof(struct usb_string_language_descriptor));
		rc = usb_get_string_descriptor(usb_device, 0, 0, lang_descriptor, sizeof(struct usb_string_language_descriptor));
		if (rc != 0) 
		{
			printk("EHCI: device string language descriptor request error (%i)!\n", rc);	
			return -1;
		}

		usb_device->lang_id = lang_descriptor->lang_ids[0];
    	kfree(lang_descriptor);
	}

	// Создать объект композитного устройства
	struct device* composite_dev = new_device();
	//device_set_name(composite_dev, usb_device->product);
	composite_dev->dev_type = DEVICE_TYPE_USB_COMPOSITE;
	composite_dev->dev_bus = DEVICE_BUS_USB;
	composite_dev->usb_info.usb_device = usb_device;
	device_set_parent(composite_dev, hci->controller_dev);

	// Обработка всех конфигураций
	for (uint8_t i = 0; i < device_descriptor.bNumConfigurations; i ++)
	{
		struct usb_configuration_descriptor* config_descriptor = NULL;
		// Считать конфигурацию по номеру
		rc = usb_device_get_configuration_descriptor(usb_device, i, &config_descriptor);
		if (rc != 0) 
		{
			printk("EHCI: device configuration descriptor (%i) request error (%i)!\n", i, rc);	
			kfree(config_descriptor);
			return -1;
		}

		// Включаем конфигурацию
		rc = usb_device_set_configuration(usb_device, config_descriptor->bConfigurationValue);
		if (rc != 0) 
		{
			printk("EHCI: device configuration descriptor setting (%i) error (%i)!\n", i, rc);	
			kfree(config_descriptor);
			return -1;
		}

		struct usb_config *conf = usb_parse_configuration_descriptor(config_descriptor);

		// Сохранить
		usb_device->configs[i] = conf;

		// Добавляем интерфейсы как устройства
		for (uint8_t iface_i = 0; iface_i < conf->descriptor.bNumInterfaces; iface_i ++)
		{
			struct usb_interface* iface = conf->interfaces[iface_i];

			struct device* usb_dev = new_device();
			//device_set_name(usb_dev, usb_device->product);
			usb_dev->dev_type = DEVICE_TYPE_USB_INTERFACE;
			usb_dev->dev_bus = DEVICE_BUS_USB;
			device_set_data(usb_dev, ehci_dev);
			usb_dev->usb_info.usb_device = usb_device;
			usb_dev->usb_info.usb_interface = iface;
			device_set_parent(usb_dev, composite_dev);

			register_device(usb_dev);
		}

		// Удаляем больше не нужный дескриптор
    	kfree(config_descriptor);    
	}
}

void ehci_check_ports(struct ehci_controller* hci)
{
	int rc;
	uint32_t port_reg;
	uint32_t portsc;
	for (int port_i = 0; port_i < hci->nports; port_i ++)
	{
		port_reg = EHCI_REG_OP_PORTSC + port_i * 4;

		portsc = ehci_op_read32(hci, port_reg);

		if ((portsc & EHCI_PORTSC_CONNECT_STATUS_CHANGE) == 0)
		{
			// Состояние порта не изменялось, пропускаем
			continue;
		}
		else
		{
			// Снимаем бит изменения статуса
			ehci_op_write32(hci, port_reg, portsc | EHCI_PORTSC_CONNECT_STATUS_CHANGE);
		}

		int is_connected = (portsc & EHCI_PORTSC_CONNECT_STATUS) == EHCI_PORTSC_CONNECT_STATUS;

		// получаем Line Status. 
		// Тут мы можем еще до выполнения RESET понять, не является ли устройство Low Speed и сразу отдать компаньону
		uint32_t LS = (portsc >> 10) & 0b11;
		printk("EHCI: Device in port %i new connect status %i. LS %i\n", port_i, is_connected, LS);

		// Если порт пустой, то дальше можно не продолжать
		if (is_connected == FALSE)
			continue;

		if (LS == EHCI_PORTSC_LINE_STATUS_K)
		{
			// это Low Speed устройство, с которым EHCI не может работать
			portsc = ehci_op_read32(hci, port_reg);
			// Передадим компаниону
			ehci_op_write32(hci, port_reg, portsc | EHCI_PORTSC_PORT_OWNER);
		}

		rc = ehci_reset_port(hci, port_reg);
		printk("EHCI: port reset result %i\n", rc);
		if (rc == 0)
		{
			// RESET успешен, устройство вставлено и является High Speed устройством
			ehci_init_device(hci, port_i);
		}
		else if (rc == 2)
		{
			// Reset неуспешен, устройства либо нет, либо оно является Low/Full Speed
			portsc = ehci_op_read32(hci, port_reg);
			// Передадим компаниону
			ehci_op_write32(hci, port_reg, portsc | EHCI_PORTSC_PORT_OWNER);
		}
	}
}

void ehci_irq_handler(void* regs, struct ehci_controller* dev) 
{
	uint32_t usbsts = ehci_op_read32(dev, EHCI_REG_OP_USBSTS);
	//printk("EHCI IRQ %x\n", usbsts);

	if ((usbsts & EHCI_USBSTS_USBINT) == EHCI_USBSTS_USBINT)
	{
		//printk("EHCI: USBINT\n");
	}

	if ((usbsts & EHCI_USBSTS_PORT_CHANGE_DETECT) == EHCI_USBSTS_PORT_CHANGE_DETECT)
	{
		dev->port_status_changed = 1;
	}

	if ((usbsts & EHCI_USBSTS_HOST_SYSTEM_ERROR) == EHCI_USBSTS_HOST_SYSTEM_ERROR)
	{
		printk("EHCI: Host System Error\n");
	}

	if ((usbsts & EHCI_USBSTS_USBERRINT) == EHCI_USBSTS_USBERRINT)
	{
		printk("EHCI: Error Interrupt\n");
	}

	ehci_op_write32(dev, EHCI_REG_OP_USBSTS, usbsts);
}

void ehci_event_thread_routine(struct ehci_controller* hci)
{
	int rc;
	while (1)
	{
		// немного поспим, чтобы не грузить процессор
		sys_thread_sleep(0, 500);

		if (hci->port_status_changed == 1) 
		{
			printk("EHCI: Port Change Detect\n");
			ehci_check_ports(hci);

			hci->port_status_changed = 0;
		}
	}
}

int ehci_device_probe(struct device *dev) 
{
	int i = 0;
	int rc;
	struct pci_device_info* device_desc = dev->pci_info;

	printk("EHCI controller found on bus: %i, device: %i func: %i\n", 
		device_desc->bus,
		device_desc->device,
		device_desc->function);

	struct pci_device_bar* BAR = &device_desc->BAR[0];

	int msi_capable = pci_device_is_msi_capable(dev->pci_info);
	int msix_capable = pci_device_is_msix_capable(dev->pci_info);
	printk("EHCI: MSI capable: %i MSI-X capable: %i\n", msi_capable, msix_capable);

	pci_set_command_reg(dev->pci_info, 
	pci_get_command_reg(dev->pci_info) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
    pci_device_set_enable_interrupts(dev->pci_info, 0);

	struct ehci_controller* cntrl = kmalloc(sizeof(struct ehci_controller));
	memset(cntrl, 0, sizeof(struct ehci_controller));

	// Сохраним указатель на объект устройства в системе
	cntrl->controller_dev = dev;
	// Разметить и сохранить BAR
	cntrl->bar_type = BAR->type;
	cntrl->mmio_addr_phys = BAR->address;
    cntrl->mmio_addr = map_io_region(cntrl->mmio_addr_phys, BAR->size);

	// Считаем CAPLEN
	uint8_t caplen = ehci_read8(cntrl, EHCI_REG_CAPLENGTH);
	uint16_t hc_version = ehci_read16(cntrl, EHCI_REG_HCIVERSION);
	printk("EHCI: CAPLEN %i, version %i.%i\n", caplen, (hc_version >> 8), (hc_version >> 4) & 0xF);
	cntrl->op_offset = caplen;

	uint32_t hccparams = ehci_read32(cntrl, EHCI_REG_HCCPARAMS);
	cntrl->mode64 = hccparams & EHCI_HCCPARAMS_64BIT;
	uint32_t eecp = (hccparams >> 8) & 0xFF;
	int pfl = (hccparams & EHCI_HCCPARAMS_PFL) == EHCI_HCCPARAMS_PFL;
	printk("EHCI: HCCPARAMS 0x%x. 64bit = %i, EECP %x, PFL = %i\n", hccparams, cntrl->mode64, eecp, pfl);

	uint32_t hcsparams = ehci_read32(cntrl, EHCI_REG_HCSPARAMS);
	cntrl->nports = hcsparams & EHCI_HCSPARAMS_NPORTS_MASK;
	uint8_t ppc = hcsparams & EHCI_HCSPARAMS_PPC;
	uint8_t N_CC = (hcsparams >> 12) & 0b1111;
	printk("EHCI: HCSPARAMS 0x%x. NPORTS %i, PPC %i, N_CC %i\n", hcsparams, cntrl->nports, ppc, N_CC);

	rc = ehci_allocate_pool(cntrl, 256, 256);
	if (rc != 0)
	{
		printk("EHCI: Failed to allocate QH and TD pool\n");
		return -1;
	}

	// Выделить память под frame list
	size_t frame_list_sz = PAGE_SIZE / sizeof(struct ehci_flp);
	cntrl->frame_list_phys = pmm_alloc(PAGE_SIZE, NULL);
	if (cntrl->frame_list_phys == NULL || cntrl->frame_list_phys > UINT32_MAX)
	{
		printk("EHCI: frame list allocation error\n");
		return -1;
	}

	cntrl->frame_list = map_io_region(cntrl->frame_list_phys, PAGE_SIZE);
	memset(cntrl->frame_list, 0, PAGE_SIZE);

	// Выделяем QH для periodic
	struct ehci_qh *periodic_qh = ehci_alloc_qh(cntrl);
	periodic_qh->link_ptr.terminate = 1;
	periodic_qh->next.terminate = 1;
	uintptr_t periodic_qh_phys = vmm_get_physical_address(periodic_qh);

	// Заполняем periodic frame list выделенной QH
	for (i = 0; i < frame_list_sz; i ++) 
	{
        cntrl->frame_list[i].type = EHCI_TYPE_QH;
        cntrl->frame_list[i].lp = periodic_qh_phys >> 5;
        cntrl->frame_list[i].terminate = 0;
    }
	cntrl->frame_list[frame_list_sz - 1].terminate = 1;

	// Выделяем QH для async
	struct ehci_qh *async_qh = ehci_alloc_qh(cntrl);
	uintptr_t async_qh_phys = vmm_get_physical_address(async_qh);
	async_qh->link_ptr.type = EHCI_TYPE_QH;
	async_qh->link_ptr.lp = async_qh_phys >> 5;
	async_qh->link_ptr.terminate = 1;
	async_qh->next.terminate = 1;
	async_qh->ep_ch.h = 1;
	async_qh->next_ptr = async_qh;

	cntrl->async_head = async_qh;

	// отобрать у BIOS
	if (eecp >= 0x40)
	{
		uint32_t usblegsup = pci_config_read32(dev, eecp + EHCI_USBLEGSUP);
		if ((usblegsup & USBLEGSUP_HC_BIOS) == USBLEGSUP_HC_BIOS)
		{
			printk("EHCI: BIOS owns controller. Performing handoff\n");
			pci_config_write32(dev, eecp + EHCI_USBLEGSUP, usblegsup | USBLEGSUP_HC_OS);

			while (1)
			{
				usblegsup = pci_config_read32(dev, eecp + EHCI_USBLEGSUP);
				if (((usblegsup & USBLEGSUP_HC_BIOS) == 0) && ((usblegsup & USBLEGSUP_HC_OS) == USBLEGSUP_HC_OS))
					break;
				hpet_sleep(1);
			}
		}
	}
	
	// сбросить контроллер перед работой
	rc = ehci_reset(cntrl);
	if (rc != 0)
	{
		printk("EHCI: Controller reset failed with code %i\n", rc);
		return -1;
	}

	int irq;
	if (msi_capable)
	{
		irq = alloc_irq(0, "ehci");
    	pci_device_set_msi_vector(dev, irq);
	}
	else
	{
		irq = pci_device_get_irq_line(dev->pci_info);
		pci_device_set_enable_interrupts(dev->pci_info, 1);
	}
    printk("EHCI: IRQ %i\n", irq);
    rc = register_irq_handler(irq, ehci_irq_handler, cntrl);
	if (rc != 0)
	{
		printk("EHCI: Error registering IRQ Handler %i\n", rc);
	}

	printk("EHCI: Starting controller\n");

	ehci_op_write32(cntrl, EHCI_REG_OP_CTRLDSSEGMENT, 0);
	// настроить принимаемые прерывания
	ehci_op_write32(cntrl, EHCI_REG_OP_USBINTR,
		EHCI_USBINTR_INT_ENABLE | EHCI_USBINTR_ERR_INT_ENABLE | EHCI_USBINTR_PORT_CHANGE_INT | EHCI_USBINTR_HSE);

	// устанавливаем адреса QH
	ehci_op_write32(cntrl, ECHI_REG_OP_PERIODICLISTBASE, cntrl->frame_list_phys);
	ehci_op_write32(cntrl, EHCI_REG_OP_ASYNCLISTADDR, async_qh_phys);
	ehci_op_write32(cntrl, EHCI_REG_OP_FRINDEX, 0);

	// формируем флаги для usbcmd для включения контроллера
	uint32_t usbcmd_run = EHCI_USBCMD_RS | EHCI_USBCMD_PERIODIC_ENABLE | EHCI_USBCMD_ASYNC_ENABLE;
	// также настроим максимальную частоту прерываний на 8 microframes (1 ms)
	usbcmd_run |= (8ULL << EHCI_USBCMD_ITC_SHIFT);

	// включаем
	ehci_op_write32(cntrl, EHCI_REG_OP_USBCMD, usbcmd_run);
	
	// ждем старта
	uint32_t usbsts;
	while (((usbsts = ehci_op_read32(cntrl, EHCI_REG_OP_USBSTS)) & EHCI_USBSTS_HCHALTED) == EHCI_USBSTS_HCHALTED)
	{
		//printk("EHCI: Wait for start %x\n", usbsts);
		hpet_sleep(1);
	}

	ehci_op_write32(cntrl, EHCI_REG_OP_CONFIGFLAG, 1);

	printk("EHCI: Controller started\n");

	// на всякий случай запустим скан портов в первый раз руками, потом уже прерываниями
	cntrl->port_status_changed = 1;

	// Процесс и поток с обраюотчиком событий
	struct process* ehci_process = create_new_process(NULL);
	process_set_name(ehci_process, "ehci");
    struct thread* ehci_event_thr = create_kthread(ehci_process, ehci_event_thread_routine, cntrl);
	thread_set_name(ehci_event_thr, "ehci_event_thread");
    scheduler_add_thread(ehci_event_thr);
	process_add_to_list((struct process*) ehci_event_thr);

    return 0;
}

struct pci_device_id ehci_ctrl_ids[] = {
	{PCI_DEVICE_CLASS(0xC, 0x3, 0x20)},
	{0,}
};

struct device_driver_ops ehci_ctrl_ops = {

    .probe = ehci_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_driver ehci_ctrl_driver = {
	.dev_name = "USB 2.0 (EHCI) Host Controller",
	.pci_device_ids = ehci_ctrl_ids,
	.ops = &ehci_ctrl_ops
};