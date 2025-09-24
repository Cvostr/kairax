#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/device_drivers.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "kairax/string.h"
#include "cdc_eth.h"
#include "kairax/errors.h"
//#include "dev/bus/usb/usb.h"

#define CDC_COMMUNICATIONS 0x2
#define CDC_DATA 0xA

#define ECM 0x6

#define SET_ETHERNET_MULTICAST_FILTERS					0x40
#define SET_ETHERNET_POWER_MANAGEMENT_PATTERN FILTER 	0x41
#define GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER	0x42
#define SET_ETHERNET_PACKET_FILTER 	0x43
#define GET_ETHERNET_STATISTIC 		0x44

// Коды дескрипторов
#define CDC_DESCTYPE_CS_INTERFACE 	0x24
#define CDC_DESCSUBTYPE_HEADER		0x00
#define CDC_DESCSUBTYPE_UNION		0x06
#define CDC_DESCSUBTYPE_ETHERNET	0x0F

// Коды прерываний
#define NETWORK_CONNECTION 0x00
#define RESPONSE_AVAILABLE 0x01
#define CONNECTION_SPEED_CHANGE 0x2A

#define PACKET_TYPE_PROMISCUOUS		1
#define PACKET_TYPE_ALL_MULTICAST	2
#define PACKET_TYPE_DIRECTED		4
#define PACKET_TYPE_BROADCAST		8
#define PACKET_TYPE_MULTICAST		16

#define LOG_DEBUG_MAC

struct usb_device_id cdc_eth_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS,
        .bInterfaceClass = CDC_COMMUNICATIONS,
		.bInterfaceSubclass = ECM,
        .bInterfaceProtocol = 0
    },
	{0,}
};

struct usb_cdc_header {
	struct usb_descriptor_header header;
    uint8_t     bDescriptorSubType; 
} PACKED;

struct usb_cdc_header_desc {
	struct usb_cdc_header header;
    uint16_t    bcdCDC;             
} PACKED;

struct cdc_union_descriptor {
	struct usb_cdc_header header;
    uint8_t     bMasterInterface0;  
    uint8_t     bSlaveInterface0;  
} PACKED;

struct cdc_ethernet_descriptor {
	struct usb_cdc_header header;
    uint8_t     iMACAddress;  
    uint32_t    bmEthernetStatistics;
	uint16_t	wMaxSegmentSize;
	uint16_t	wNumberMCFilters;
	uint8_t		bNumberPowerFilters;  
} PACKED;

struct cdc_notification_header {
	uint8_t bmRequestType;
	uint8_t bNotificationCode;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint8_t  data[];
} PACKED;

struct cdc_conn_speed_change {
	uint32_t	DLBitRate;
	uint32_t	ULBitRate;	
};

struct cdc_eth_dev {
	struct nic* iface;
	struct usb_device* usbdev;

	struct usb_endpoint* interrupt_in;
	struct usb_endpoint* data_in;
	struct usb_endpoint* data_out;	

	struct cdc_notification_header*     notification_buffer;
	size_t    report_buffer_pages;
    uintptr_t report_buffer_phys;

	size_t rx_pages;
	uintptr_t rx_data_buffer_phys;
    uintptr_t rx_data_buffer;

	int conn_state;
};

int usb_cdc_hex2int(char hex) 
{
    if (hex >= '0' && hex <= '9')
        return hex - '0';
    if (hex >= 'A' && hex <= 'F')
        return hex - 'A' + 10;

    return -1;
}

int usb_cdc_set_eth_packet_filter(struct usb_device* device, struct usb_interface* interface, uint16_t filter) 
{
	struct usb_device_request lun_req;
	lun_req.type = USB_DEVICE_REQ_TYPE_CLASS;
	lun_req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	lun_req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	lun_req.bRequest = SET_ETHERNET_PACKET_FILTER;
	lun_req.wValue = filter;
	lun_req.wIndex = interface->descriptor.bInterfaceNumber;
	lun_req.wLength = 0;

    return usb_device_send_request(device, &lun_req, NULL, 0);
}

int usb_cdc_set_altsetting(struct usb_device* device, struct usb_interface* altinterface) 
{
	return usb_set_interface(device, altinterface->descriptor.bInterfaceNumber, altinterface->descriptor.bAlternateSetting);
}

void usb_cdc_notify_handler(struct usb_msg* msg)
{
    struct cdc_eth_dev* ethdev = msg->private;
	struct cdc_notification_header *header = ethdev->notification_buffer;

	switch (header->bNotificationCode)
	{
		case NETWORK_CONNECTION:
			int new_conn_state = header->wValue;
			if (new_conn_state != ethdev->conn_state)
			{
				if (new_conn_state == 0)
				{
					ethdev->iface->flags |= NIC_FLAG_NO_CARRIER;
				}
				else if (new_conn_state == 1)
				{
					ethdev->iface->flags &= ~(NIC_FLAG_NO_CARRIER);
				}
			}

			ethdev->conn_state = new_conn_state;

			break;
		case CONNECTION_SPEED_CHANGE:
			struct cdc_conn_speed_change* speedval = (struct cdc_conn_speed_change*) header->data;
			//printk("CDC ECM: Speed Change DLBitRate = %i, ULBitRate = %i\n", speedval->DLBitRate, speedval->ULBitRate);
			break;
		case RESPONSE_AVAILABLE:
			printk("CDC ECM: Response\n");
			break;
		default:
			printk("CDC ECM: notification %x\n", header->bNotificationCode);
	}

	// Ожидание прерывания
    usb_send_async_msg(ethdev->usbdev, ethdev->interrupt_in, msg);
}

void usb_cdc_rx_callback(struct usb_msg* msg)
{
	printk("CDC Rx callback");
}

void usb_cdc_rx_thread(struct cdc_eth_dev* eth_dev)
{
	struct usb_msg *msg = new_usb_msg();

	msg->data = eth_dev->rx_data_buffer_phys;
	msg->length = eth_dev->iface->mtu;
	//msg->callback = usb_cdc_rx_callback;
	msg->private = eth_dev;

	while (1)
	{
		int rc = usb_send_async_msg(eth_dev->usbdev, eth_dev->data_in, msg);

		while (msg->status == -EINPROGRESS)
		{

		}

		uint32_t received_bytes = msg->transferred_length;
	
		if (received_bytes > 0)
		{
			struct net_buffer* nb = new_net_buffer(eth_dev->rx_data_buffer, received_bytes, eth_dev->iface);
			net_buffer_acquire(nb);
			eth_handle_frame(nb);
			net_buffer_free(nb);
		}
	}
}

int usb_cdc_tx(struct nic* nic, const unsigned char* buffer, size_t size)
{
	struct cdc_eth_dev* eth_dev = nic->dev->dev_data;

	// Выделить временную память с запретом кэширования
	size_t reqd_pages = 0;
	uintptr_t tmp_data_buffer_phys = (uintptr_t) pmm_alloc(size, &reqd_pages);
    uintptr_t tmp_data_buffer = map_io_region(tmp_data_buffer_phys, reqd_pages * PAGE_SIZE);

	memcpy(tmp_data_buffer, buffer, size);

	// Отправить
	int rc = usb_device_bulk_msg(eth_dev->usbdev, eth_dev->data_out, tmp_data_buffer_phys, size);

	// Очистка
	unmap_io_region(tmp_data_buffer, reqd_pages * PAGE_SIZE);
    pmm_free_pages(tmp_data_buffer_phys, reqd_pages);

	return 0;
}

int usb_cdc_device_probe(struct device *dev) 
{
	size_t i;
    struct usb_interface* interface = dev->usb_info.usb_interface;
    struct usb_device* device = dev->usb_info.usb_device;

	printk("CDC ECM Ethernet device detected\n");

	// Найдем основные дескрипторы
	struct usb_cdc_header_desc *header_desc = NULL;
	struct cdc_union_descriptor *union_desc = NULL;
	struct cdc_ethernet_descriptor *ethernet_desc = NULL;

	for (i = 0; i < interface->other_descriptors_num; i ++)
	{
		struct usb_cdc_header *cdc_header = (struct usb_cdc_header *) interface->other_descriptors[i];
		if (cdc_header->header.bDescriptorType == CDC_DESCTYPE_CS_INTERFACE)
		{
			switch (cdc_header->bDescriptorSubType)
			{
				case CDC_DESCSUBTYPE_HEADER:
					header_desc = (struct usb_cdc_header_desc *) cdc_header;
					break;
				case CDC_DESCSUBTYPE_UNION:
					union_desc = (struct cdc_union_descriptor *) cdc_header;
					break;
				case CDC_DESCSUBTYPE_ETHERNET:
					ethernet_desc = (struct cdc_ethernet_descriptor *) cdc_header;
					break;
			}
		}
	}

	if (header_desc == NULL || union_desc == NULL || ethernet_desc == NULL)
	{
		printk("CDC ECM: One of mandatory headers is not present\n");
		return -1;
	}

	//printk("CDC: Master %i Slave %i\n", union_desc->bMasterInterface0, union_desc->bSlaveInterface0);
	printk("ECM: iMAC %i, MTU %i\n", ethernet_desc->iMACAddress, ethernet_desc->wMaxSegmentSize);

	uint16_t MTU = ethernet_desc->wMaxSegmentSize;

	// Получение MAC адреса из строки
	uint8_t MAC_STR[6 * 2 * 2];
	usb_get_string(device, ethernet_desc->iMACAddress, MAC_STR, sizeof(MAC_STR));

	// Приведение MAC адреса к бинарному виду
	uint8_t MAC[6];
	for (i = 0; i < 6; i ++)
	{
		uint8_t c1 = MAC_STR[i * 4];
		uint8_t c2 = MAC_STR[i * 4 + 2];
		MAC[i] = (usb_cdc_hex2int(c1) << 4) | usb_cdc_hex2int(c2); 
	}

#ifdef LOG_DEBUG_MAC
	printk ("ECM: MAC %x:%x:%x:%x:%x:%x\n", 
		MAC[0], 
		MAC[1],
		MAC[2],
		MAC[3],
		MAC[4],
		MAC[5]);
#endif

	uint16_t iface_string[20];
	// Считаем строку интерфейса, вдруг устройству это очень важно
	usb_get_string(device, interface->descriptor.iInterface, iface_string, sizeof(iface_string));

	struct usb_interface* cdc_data_interface = NULL;
	
	// Найти интерфейс с CDC Data
	struct usb_config* usb_conf = interface->parent_config;
	for (i = 0; i < usb_conf->interfaces_num; i ++)
	{
		struct usb_interface* iface = usb_conf->interfaces[i];
		if (iface->descriptor.bInterfaceClass == CDC_DATA && iface->descriptor.bNumEndpoints == 2)
		{
			cdc_data_interface = iface;
			if (cdc_data_interface->descriptor.bAlternateSetting != 0)
			{
				usb_cdc_set_altsetting(device, cdc_data_interface);
			}
			break;
		}
	}

	if (cdc_data_interface == NULL)
	{
		printk("CDC ECM: No CDC-DATA interface\n");
		return -1;
	}

	// Считаем строку интерфейса, вдруг устройству это очень важно
	usb_get_string(device, cdc_data_interface->descriptor.iInterface, iface_string, sizeof(iface_string));

	size_t interrupt_max_packet_sz = 0;
	struct usb_endpoint* interrupt_in = NULL;
	struct usb_endpoint* data_in = NULL;
	struct usb_endpoint* data_out = NULL;	

	struct usb_endpoint* ep;

	// Найти interrupt endpoint в основном интерфейсе
	for (i = 0; i < interface->descriptor.bNumEndpoints; i ++)
	{
		ep = &interface->endpoints[i];

		// Нам интересны только Interrupt
		if ((ep->descriptor.bmAttributes & USB_ENDPOINT_ATTR_TT_MASK) != USB_ENDPOINT_ATTR_TT_INTERRUPT)
			continue;

        // и только IN
		if ((ep->descriptor.bEndpointAddress & USB_ENDPOINT_ADDR_DIRECTION_IN) == USB_ENDPOINT_ADDR_DIRECTION_IN)
		{
			interrupt_in = ep;
			interrupt_max_packet_sz = interrupt_in->descriptor.wMaxPacketSize & 0x07FF;
			break;
		}
	}

	// Найти BULK эндпоинты
	for (i = 0; i < cdc_data_interface->descriptor.bNumEndpoints; i ++)
	{
		ep = &cdc_data_interface->endpoints[i];

		// Нам интересны только Bulk
		if ((ep->descriptor.bmAttributes & USB_ENDPOINT_ATTR_TT_MASK) != USB_ENDPOINT_ATTR_TT_BULK)
			continue;

		if ((ep->descriptor.bEndpointAddress & USB_ENDPOINT_ADDR_DIRECTION_IN) == USB_ENDPOINT_ADDR_DIRECTION_IN)
		{
			// IN
			data_in = ep;
		}
		else 
		{
			// OUT
			data_out = ep;
		}
	}

	// Не найдены необходимые endpoints
	if (interrupt_in == NULL)
	{
		printk("CDC ECM: No CDC interrupt endpoint\n");
		return -1;
	}

	if (data_out == NULL || data_in == NULL)
	{
		printk("CDC ECM: No CDC-DATA bulk endpoints\n");
		return -1;
	}

	// Отправить SetPacketFilter
	int rc = usb_cdc_set_eth_packet_filter(device, interface, 
		(PACKET_TYPE_PROMISCUOUS | PACKET_TYPE_ALL_MULTICAST | PACKET_TYPE_DIRECTED | PACKET_TYPE_BROADCAST));
	if (rc != 0)
	{
		printk("CDC ECM: Error in SET_ETHERNET_PACKET_FILTER (%i)\n", rc);
		return -1;
	}

	// Инициализировать Interrupt
	rc = usb_device_configure_endpoint(device, interrupt_in);
	if (rc != 0)
	{
		printk("CDC ECM: Error configuring IN interrupt endpoint (%i)\n", rc);
		return -1;
	}

	printk("CDC ECM: Interrupt EP max packet size %i\n", interrupt_max_packet_sz);

	// Инициализируем BULK
	rc = usb_device_configure_endpoint(device, data_in);
	if (rc != 0)
	{
		printk("CDC ECM: Error initialize IN endpoint (%i)\n", rc);
		return -1;
	}

	rc = usb_device_configure_endpoint(device, data_out);
	if (rc != 0)
	{
		printk("CDC ECM: Error initialize OUT endpoint (%i)\n", rc);
		return -1;
	}

	struct cdc_eth_dev* eth_dev = kmalloc(sizeof(struct cdc_eth_dev));
	eth_dev->interrupt_in = interrupt_in;
	eth_dev->data_in = data_in;
	eth_dev->data_out = data_out;
	eth_dev->usbdev = device;
	// Выделить память под буфер report
    eth_dev->report_buffer_phys = (uintptr_t) pmm_alloc(interrupt_max_packet_sz, &eth_dev->report_buffer_pages);
    eth_dev->notification_buffer = map_io_region(eth_dev->report_buffer_phys, eth_dev->report_buffer_pages * PAGE_SIZE);

	// Регистрация интерфейса в ядре
    eth_dev->iface = new_nic();
    memcpy(eth_dev->iface->mac, MAC, MAC_DEFAULT_LEN);
    eth_dev->iface->dev = dev; 
    eth_dev->iface->flags = NIC_FLAG_UP | NIC_FLAG_BROADCAST | NIC_FLAG_MULTICAST | NIC_FLAG_NO_CARRIER;
    eth_dev->iface->tx = usb_cdc_tx;
    //eth_dev->nic->up = e1000_up;
    //eth_dev->nic->down = e1000_down;
    eth_dev->iface->mtu = MTU;
    register_nic(eth_dev->iface, "eth");

    dev->dev_type = DEVICE_TYPE_NETWORK_ADAPTER;
    dev->dev_data = eth_dev;
    dev->nic = eth_dev->iface;

	// Rx буфер
	eth_dev->rx_data_buffer_phys = (uintptr_t) pmm_alloc(MTU, &eth_dev->rx_pages);
    eth_dev->rx_data_buffer = map_io_region(eth_dev->rx_data_buffer_phys, eth_dev->rx_pages * PAGE_SIZE);

	struct usb_msg* msg = kmalloc(sizeof(struct usb_msg));
    msg->data = eth_dev->report_buffer_phys;
    msg->length = interrupt_max_packet_sz;
    msg->callback = usb_cdc_notify_handler;
    msg->private = eth_dev;

	// Ожидание прерывания
    usb_send_async_msg(device, interrupt_in, msg);

	struct process* proc = create_new_process(NULL);
    struct thread* thr = create_kthread(proc, usb_cdc_rx_thread, eth_dev);
    scheduler_add_thread(thr);
	
	return 0;
}

struct device_driver_ops cdc_eth_ops = {

    .probe = usb_cdc_device_probe,
    //.remove = usb_mass_device_remove
    //void (*shutdown) (struct device *dev);
};

struct usb_device_driver usb_cdc_eth_driver = {
	.device_ids = cdc_eth_ids,
	.ops = &cdc_eth_ops
};

int cdc_eth_init(void)
{
    register_usb_device_driver(&usb_cdc_eth_driver);
	
	return 0;
}

void cdc_eth_exit(void)
{

}

DEFINE_MODULE("cdc_eth", cdc_eth_init, cdc_eth_exit)