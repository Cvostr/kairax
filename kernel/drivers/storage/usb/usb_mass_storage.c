#include "dev/device_man.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "mem/iomem.h"
#include "string.h"
#include "kairax/kstdlib.h"
#include "kairax/in.h"
#include "kairax/stdio.h"
#include "kairax/errors.h"

struct command_block_wrapper {
    uint32_t signature;
    uint32_t tag;
    uint32_t length;
    uint8_t flags;
    uint8_t lun;
    uint8_t command_len;
    uint8_t data[16];
} PACKED;

struct command_status_wrapper {
    uint32_t signature;
    uint32_t tag;
    uint32_t data_residue;
    uint8_t status;
} PACKED;

struct inquiry_block {
    uint8_t periphal_device_type:5;
    uint8_t peripheral_qualifier:3;
    uint8_t reserved1:7;
    uint8_t RMB:1;
    uint8_t version;
} PACKED;

struct inquiry_block_result
{
	uint8_t peripheral_device_type : 5;
	uint8_t peripheral_qualifier   : 3;

	uint8_t reserved0 : 7;
	uint8_t rmb       : 1;

	uint8_t version;

	uint8_t response_data_format : 4;
	uint8_t hisup                : 1;
	uint8_t normaca              : 1;
	uint8_t obsolete0            : 1;
	uint8_t obsolete1            : 1;

	uint8_t additional_length;

	uint8_t protect   : 1;
	uint8_t reserved1 : 2;
	uint8_t _3pc      : 1;
	uint8_t tgps      : 2;
	uint8_t acc       : 1;
	uint8_t sccs      : 1;

	uint8_t obsolete2 : 1;
	uint8_t obsolete3 : 1;
	uint8_t obsolete4 : 1;
	uint8_t obsolete5 : 1;
	uint8_t multip    : 1;
	uint8_t vs0       : 1;
	uint8_t encserv   : 1;
	uint8_t obsolete6 : 1;

	uint8_t vs1       : 1;
	uint8_t cmdque    : 1;
	uint8_t obsolete7 : 1;
	uint8_t obsolete8 : 1;
	uint8_t obsolete9 : 1;
	uint8_t obsolete10 : 1;
	uint8_t obsolete11 : 1;
	uint8_t obsolete12 : 1;

	uint8_t t10_vendor_identification[8];
	uint8_t product_identification[16];
	uint8_t product_revision_level[4];
} PACKED;

struct read_capacity_result
{
	uint32_t logical_block_address;
	uint32_t block_length;
} PACKED;

#define CBW_SIGNATURE  	0x43425355
#define CSW_SIGNATURE	0x53425355

#define SCSI_TEST_UNIT_READY	0x00
#define SCSI_INQUIRY			0x12
#define SCSI_READ_CAPACITY      0x25
#define SCSI_READ_10 			0x28
#define SCSI_WRITE_10 			0x2A
#define SCSI_READ_12			0xA8
#define SCSI_WRITE_12			0xAA
#define SCSI_READ_16			0x88
#define SCSI_WRITE_16			0x8A

#define CBW_TO_DEVICE	0x00
#define CBW_TO_HOST		0x80

#define CSW_SUCCESS		0x00
#define CSW_FAILED		0x01
#define CSW_PHASE_ERROR	0x02

#define USB_REQ_RESET		0xFF
#define USB_REQ_LUN_INFO 	0xFE

#define CBW_SIZE 			sizeof(struct command_block_wrapper)

struct usb_mass_storage_device {

	uint8_t lun_id;
	struct usb_device* usb_dev;
	struct usb_interface* usb_iface;
	struct usb_endpoint* in_ep;
	struct usb_endpoint* out_ep;

	uint32_t blocksize;

	uint8_t peripheral_device_type : 5;
	uint8_t peripheral_qualifier   : 3;
};

struct usb_device_id usb_mass_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .bInterfaceClass = 0x08,
		.bInterfaceSubclass = 0x06,
        .bInterfaceProtocol = 0x50
    },
	{0,}
};

int usb_mass_device_reset(struct usb_device* device, struct usb_interface* interface)
{
	struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = USB_REQ_RESET;
	req.wValue = 0;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = 0; // Данный вид запроса не имеет выходных данных

    return usb_device_send_request(device, &req, NULL, 0);
}

int usb_mass_device_get_max_lun(struct usb_device* device, struct usb_interface* interface, uint8_t* max_lun)
{
	struct usb_device_request lun_req;
	lun_req.type = USB_DEVICE_REQ_TYPE_CLASS;
	lun_req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	lun_req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	lun_req.bRequest = USB_REQ_LUN_INFO;
	lun_req.wValue = 0;
	lun_req.wIndex = interface->descriptor.bInterfaceNumber;
	lun_req.wLength = 1;

    return usb_device_send_request(device, &lun_req, max_lun, 1);
}

int usb_mass_device_clear_feature(struct usb_device* device, struct usb_endpoint* ep)
{
	struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_STANDART;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_ENDPOINT;
	req.bRequest = USB_DEVICE_REQ_CLEAR_FEATURE;
	req.wValue = 0;
	// Разметка полей идентична
	req.wIndex = ep->descriptor.bEndpointAddress;
	req.wLength = 0;

    return usb_device_send_request(device, &req, NULL, 0);
}

int usb_mass_reset_recovery(struct usb_mass_storage_device* dev)
{
	usb_mass_device_reset(dev->usb_dev, dev->usb_iface);
	usb_mass_device_clear_feature(dev->usb_dev, dev->in_ep);
	usb_mass_device_clear_feature(dev->usb_dev, dev->out_ep);

	return 0;
}

int usb_mass_bulk_msg_with_stall_recovery(struct usb_device* device, struct usb_endpoint* endpoint, void* data, uint32_t length)
{
	int rc = usb_device_bulk_msg(device, endpoint, data, length);
	if (rc == USB_COMPLETION_CODE_STALL)
	{
		printk("USB Mass: STALL\n");
		// STALL - штатная ситуация. Надо отресетить эндпоинт
		rc = usb_mass_device_clear_feature(device, endpoint);
		if (rc != 0)
		{
			printk("USB Mass: Error resetting endpoint %i\n", rc);
			return rc;
		}
		// И попробовать еще раз
		rc = usb_device_bulk_msg(device, endpoint, data, length);
	}

	return rc;
}

int usb_mass_exec_cmd(struct usb_mass_storage_device* dev, struct command_block_wrapper* cbw, int is_output, void* out, uint32_t len)
{
	int rc = 0;
	size_t reqd_pages = 0;
	// Сосчитать необходимую память
	size_t reqd_bytes = CBW_SIZE;
	reqd_bytes = MAX(reqd_bytes, sizeof(struct command_status_wrapper));
	reqd_bytes = MAX(reqd_bytes, len);
	// Выделить временную память с запретом кэширования
	uintptr_t tmp_data_buffer_phys = (uintptr_t) pmm_alloc(reqd_bytes, &reqd_pages);
    uintptr_t tmp_data_buffer = map_io_region(tmp_data_buffer_phys, reqd_pages * PAGE_SIZE);

	// Сначала скопируем CBW туда
	memcpy(tmp_data_buffer, cbw, CBW_SIZE);

	// отправка CBW на выполнение
	rc = usb_device_bulk_msg(dev->usb_dev, dev->out_ep, tmp_data_buffer_phys, CBW_SIZE);
	if (rc != 0)
	{
		printk("USB Mass: Error sending CBW %i\n", rc);
		goto cleanup_exit;
	}

	if (len) 
	{
		if (is_output == FALSE)
		{
			// прием ответа
			rc = usb_mass_bulk_msg_with_stall_recovery(dev->usb_dev, dev->in_ep, tmp_data_buffer_phys, len);
			if (rc != 0)
			{
				goto cleanup_exit;
			}
			// копирование ответа
			memcpy(out, tmp_data_buffer, len);
		} 
		else 
		{
			// копирование данных
			memcpy(tmp_data_buffer, out, len);
			// отправка данных
			rc = usb_mass_bulk_msg_with_stall_recovery(dev->usb_dev, dev->out_ep, tmp_data_buffer_phys, len);
			if (rc != 0)
			{
				goto cleanup_exit;
			}
		}
	}

	// Прием данных CSW
	rc = usb_mass_bulk_msg_with_stall_recovery(dev->usb_dev, dev->in_ep, tmp_data_buffer_phys, sizeof(struct command_status_wrapper));
	if (rc != 0)
	{
		goto cleanup_exit;
	}

	struct command_status_wrapper* csw = (tmp_data_buffer);
	if (csw->signature != CSW_SIGNATURE)
	{
		printk("CSW Signature incorrect!\n");
		rc = -EIO;
		goto cleanup_exit;
	}

	switch (csw->status)
	{
		case 0x0:
		case 0x1:
			rc = 0;
			break;
		case 0x2:
			usb_mass_reset_recovery(dev);
			rc = -1;
			break;
		default:
			rc = -2;
			break;
	}

cleanup_exit:
	unmap_io_region(tmp_data_buffer, reqd_pages * PAGE_SIZE);
    pmm_free_pages(tmp_data_buffer_phys, reqd_pages);

	return rc;
}

int usb_mass_read(struct usb_mass_storage_device* dev, uint64_t sector, uint16_t count, char* out)
{
	size_t max_pckt_sz = dev->in_ep->descriptor.wMaxPacketSize;
	int rc = 0;

	// Сколько блоков можем считать за один пакет
	uint32_t blocks_per_packet = max_pckt_sz / dev->blocksize;
	// Сколько байт можем считать за один пакет
	// Вычисляем, потому что значение wMaxPacketSize может быть не кратно dev->blocksize
	uint32_t bytes_per_packet = blocks_per_packet * dev->blocksize;
	// Сколько будет пакетов
	uint32_t packets = count / blocks_per_packet;  

	if (packets % blocks_per_packet != 0)
		packets++;

	struct command_block_wrapper cbw;
	cbw.signature = CBW_SIGNATURE;
	cbw.tag = 0;
    cbw.flags = CBW_TO_HOST;
    cbw.command_len = 10;
	cbw.lun = dev->lun_id;

	uint32_t remaining_blocks = count;

	for (uint32_t i = 0; i < packets; i ++)
	{
		// Сколько блоков будем читать
		uint32_t blocks_for_read = MIN(remaining_blocks, blocks_per_packet);
		// Сколько байт будем читать?
		uint32_t bytes_for_read = blocks_for_read * dev->blocksize;

		cbw.length = bytes_for_read;

		cbw.data[0] = SCSI_READ_10;
		// reserved
		cbw.data[1] = 0;
		// lba
		cbw.data[2] = (uint8_t) ((sector >> 24) & 0xFF);
		cbw.data[3] = (uint8_t) ((sector >> 16) & 0xFF);
		cbw.data[4] = (uint8_t) ((sector >> 8) & 0xFF);
		cbw.data[5] = (uint8_t) ((sector) & 0xFF);
		// counter
		cbw.data[6] = 0;
		cbw.data[7] = (uint8_t) ((blocks_for_read >> 8) & 0xFF);
		cbw.data[8] = (uint8_t) ((blocks_for_read) & 0xFF);

		// Выполним команду
		rc = usb_mass_exec_cmd(dev, &cbw, FALSE, out + i * bytes_per_packet, bytes_for_read);
		if (rc != 0)
		{
			return -EIO;
		}

		sector += blocks_for_read;
		remaining_blocks -= blocks_for_read;
	}

	return 0;
}

int usb_mass_inquiry(struct usb_mass_storage_device* dev, struct inquiry_block_result* inq_result)
{
	struct command_block_wrapper cbw;
	memset(&cbw, 0, sizeof(struct command_block_wrapper));
	cbw.signature = CBW_SIGNATURE;
	cbw.tag = 0;
    cbw.length = sizeof(struct inquiry_block_result);
    cbw.flags = CBW_TO_HOST;
    cbw.command_len = 6;
	cbw.lun = dev->lun_id;

    cbw.data[0] = SCSI_INQUIRY;
    cbw.data[1] = 0;
    cbw.data[2] = 0;
    cbw.data[3] = 0;
    cbw.data[4] = sizeof(struct inquiry_block_result);
    cbw.data[5] = 0;

	return usb_mass_exec_cmd(dev, &cbw, FALSE, inq_result, sizeof(struct inquiry_block_result));
}

int usb_mass_test_unit_ready(struct usb_mass_storage_device* dev)
{
	struct command_block_wrapper cbw;
	memset(&cbw, 0, sizeof(struct command_block_wrapper));
	cbw.signature = CBW_SIGNATURE;
	cbw.tag = 0;
    cbw.length = 0;
    cbw.flags = CBW_TO_HOST;
    cbw.command_len = 6;
	cbw.lun = dev->lun_id;

    cbw.data[0] = SCSI_TEST_UNIT_READY;
    cbw.data[1] = 0;
    cbw.data[2] = 0;
    cbw.data[3] = 0;
    cbw.data[4] = 0;
    cbw.data[5] = 0;

	return usb_mass_exec_cmd(dev, &cbw, FALSE, NULL, 0);
}

int usb_mass_get_capacity(struct usb_mass_storage_device* dev, uint64_t* blocks, uint32_t* block_size)
{
	struct command_block_wrapper cbw;
	memset(&cbw, 0, sizeof(struct command_block_wrapper));
	cbw.signature = CBW_SIGNATURE;
	cbw.tag = 0;
    cbw.length = sizeof(struct read_capacity_result);
    cbw.flags = CBW_TO_HOST;
    cbw.command_len = 10;
	cbw.lun = dev->lun_id;

    cbw.data[0] = SCSI_READ_CAPACITY;
    cbw.data[1] = 0;
    cbw.data[2] = 0;
    cbw.data[3] = 0;
    cbw.data[4] = 0;
    cbw.data[5] = 0;

	struct read_capacity_result capacity_result;
	int rc = usb_mass_exec_cmd(dev, &cbw, FALSE, &capacity_result, sizeof(struct read_capacity_result));
	if (rc != 0)
	{
		return rc;
	}

	// Результат будет с порядком байт Big Endian
	*blocks = ntohl(capacity_result.logical_block_address) + 1;
	*block_size = ntohl(capacity_result.block_length);
	
	return 0;
}

int usb_mass_device_read_lba(struct device *device, uint64_t start, uint64_t count, const char *buf)
{
	return usb_mass_read(device->dev_data, start, (uint32_t) count, buf);
}

int usb_mass_device_write_lba(struct device *device, uint64_t start, uint64_t count, char *buf)
{
	return -1;
}

int usb_mass_device_probe(struct device *dev) 
{
	struct usb_interface* interface = dev->usb_info.usb_interface;
    struct usb_device* device = dev->usb_info.usb_device;
    //printk("USB mass storage %x %x\n", interface->descriptor.bInterfaceClass, interface->descriptor.bInterfaceProtocol);

    int rc;

	rc = usb_mass_device_reset(device, interface);
	if (rc != 0)
	{
		printk("USB: Reset failed (%i)\n", rc);
		return -1;
	}

    uint8_t max_lun = 0;
	rc = usb_mass_device_get_max_lun(device, interface, &max_lun);
	if (rc != 0)
	{
		printk("USB Mass: Max LUN request failed with code %i\n", rc);
		return -1;
	}
    printk("USB Mass: max lun = %i\n", rc, max_lun);

	struct usb_endpoint* builk_in = NULL;
	struct usb_endpoint* builk_out = NULL;

	for (uint8_t i = 0; i < interface->descriptor.bNumEndpoints; i ++)
	{
		struct usb_endpoint* ep = &interface->endpoints[i];

		// Нам интересны только Bulk
		if ((ep->descriptor.bmAttributes & USB_ENDPOINT_ATTR_TT_MASK) != USB_ENDPOINT_ATTR_TT_BULK)
			continue;

		if ((ep->descriptor.bEndpointAddress & USB_ENDPOINT_ADDR_DIRECTION_IN) == USB_ENDPOINT_ADDR_DIRECTION_IN)
		{
			// IN
			builk_in = ep;
		}
		else 
		{
			// OUT
			builk_out = ep;
		}
	}

	// Не найдены необходимые endpoints
	if (builk_out == NULL || builk_in == NULL)
	{
		return -1;
	}

	rc = usb_device_configure_endpoint(device, builk_in);
	if (rc != 0)
	{
		printk("USB: Error initialize IN endpoint (%i)\n", rc);
		return -1;
	}

	rc = usb_device_configure_endpoint(device, builk_out);
	if (rc != 0)
	{
		printk("USB: Error initialize OUT endpoint (%i)\n", rc);
		return -1;
	}

	rc = usb_mass_device_clear_feature(device, builk_in);
	rc = usb_mass_device_clear_feature(device, builk_out);

	for (uint8_t lun_i = 0; lun_i < max_lun + 1; lun_i ++)
	{
		struct usb_mass_storage_device* usb_mass = kmalloc(sizeof(struct usb_mass_storage_device));
		usb_mass->in_ep = builk_in;
		usb_mass->out_ep = builk_out;
		usb_mass->lun_id = lun_i;
		usb_mass->usb_dev = device;
		usb_mass->usb_iface = interface;

		// Сначала запросим информацию
		struct inquiry_block_result inq_result;
		rc = usb_mass_inquiry(usb_mass, &inq_result);
		if (rc != 0)
		{
			kfree(usb_mass);
			printk("USB Mass: inqury error with code %i\n", rc);
			continue;
		}
		usb_mass->peripheral_device_type = inq_result.peripheral_device_type;
		usb_mass->peripheral_qualifier = inq_result.peripheral_qualifier;
/*		
		printk("INQUIRY result: %s, %s, DT %i, Q %i\n", inq_result.t10_vendor_identification, inq_result.product_identification,
			usb_mass->peripheral_device_type, usb_mass->peripheral_qualifier);
*/

		// Некоторые устройства хотят TEST UNIT
		rc = usb_mass_test_unit_ready(usb_mass);
		if (rc != 0)
		{
			kfree(usb_mass);
			printk("USB Mass: Test Unit error with code %i\n", rc);
			continue;
		}

		// Считаем объем устройства
		uint64_t blocks;
		rc = usb_mass_get_capacity(usb_mass, &blocks, &usb_mass->blocksize);
		if (rc != 0)
		{
			kfree(usb_mass);
			printk("USB Mass: get capacity error with code %i\n", rc);
			continue;
		}

		// Собрать структуру диска
		struct drive_device_info* drive_info = new_drive_device_info();
		drive_info->sectors = blocks;
		drive_info->block_size = usb_mass->blocksize;
		drive_info->nbytes = blocks * usb_mass->blocksize;

		drive_info->read = usb_mass_device_read_lba;
		drive_info->write = usb_mass_device_write_lba;
		// TEMP
		strcpy(drive_info->blockdev_name, "usb0");

		struct device* lun_dev = new_device();
		lun_dev->dev_type = DEVICE_TYPE_DRIVE;
		lun_dev->dev_parent = dev;
		lun_dev->dev_data = usb_mass;
		lun_dev->drive_info = drive_info;
		strncpy(lun_dev->dev_name, inq_result.t10_vendor_identification, 8);
		strncat(lun_dev->dev_name, inq_result.product_identification, sizeof(inq_result.product_identification));

		register_device(lun_dev);

		// TODO: move??
		add_partitions_from_device(lun_dev);
	}
}

void usb_mass_device_remove(struct device *dev) 
{
	printk("USB BBB Device Remove\n");
	return 0;
}

struct device_driver_ops usb_mass_ops = {

    .probe = usb_mass_device_probe,
    .remove = usb_mass_device_remove
    //void (*shutdown) (struct device *dev);
};

struct usb_device_driver usb_mass_storage_driver = {
	.device_ids = usb_mass_ids,
	.ops = &usb_mass_ops
};

void usb_mass_init()
{
	register_usb_device_driver(&usb_mass_storage_driver);
}
