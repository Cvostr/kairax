#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/device_drivers.h"
#include "dev/interrupts.h"
#include "mem/iomem.h"
#include "kairax/string.h"

#define CDC_COMMUNICATIONS 0x2
#define CDC_DATA 0xA

#define ECM 0x6

#define SET_ETHERNET_PACKET_FILTER 0x43

#define CDC_DESCTYPE_CS_INTERFACE 	0x24
#define CDC_DESCSUBTYPE_HEADER		0x00
#define CDC_DESCSUBTYPE_UNION		0x06
#define CDC_DESCSUBTYPE_ETHERNET	0x0F

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

int usb_cdc_device_probe(struct device *dev) 
{
	size_t i;
    struct usb_interface* interface = dev->usb_info.usb_interface;
    struct usb_device* device = dev->usb_info.usb_device;

	printk("CDC ECM Ethernet device deteced\n");

	// Найдем основные дескрипторы
	struct usb_cdc_header_desc *header_desc = NULL;
	struct cdc_union_descriptor *union_desc = NULL;
	struct cdc_ethernet_descriptor *ethernet_desc = NULL;

	for (i = 0; i < interface->other_descriptors_num; i ++)
	{
		struct usb_cdc_header *cdc_header = interface->other_descriptors[i];
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

	printk("CDC: Master %i Slave %i\n", union_desc->bMasterInterface0, union_desc->bSlaveInterface0);
	printk("ECM: iMAC %i, MTU %i\n", ethernet_desc->iMACAddress, ethernet_desc->wMaxSegmentSize);

	struct usb_interface* cdc_data_interface = NULL;
	
	// Найти интерфейс с CDC Data
	struct usb_config* usb_conf = interface->parent_config;
	for (i = 0; i < usb_conf->interfaces_num; i ++)
	{
		struct usb_interface* iface = usb_conf->interfaces[i];
		if (iface->descriptor.bInterfaceClass == CDC_DATA && iface->descriptor.bNumEndpoints == 2)
		{
			cdc_data_interface = iface;
			break;
		}
	}

	if (cdc_data_interface == NULL)
	{
		printk("CDC ECM: No CDC-DATA interface\n");
		return -1;
	}

	struct usb_endpoint* data_in = NULL;
	struct usb_endpoint* data_out = NULL;	

	// Найти BULK эндпоинты
	for (i = 0; i < cdc_data_interface->descriptor.bNumEndpoints; i ++)
	{
		struct usb_endpoint* ep = &cdc_data_interface->endpoints[i];

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
	if (data_out == NULL || data_in == NULL)
	{
		printk("CDC ECM: No CDC-DATA bulk endpoints\n");
		return -1;
	}

    struct usb_device_request lun_req;
	lun_req.type = USB_DEVICE_REQ_TYPE_CLASS;
	lun_req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	lun_req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	lun_req.bRequest = SET_ETHERNET_PACKET_FILTER;
	lun_req.wValue = 0xF;
	lun_req.wIndex = interface->descriptor.bInterfaceNumber;
	lun_req.wLength = 0;

    int rc = usb_device_send_request(device, &lun_req, NULL, 0);
	printk("SET_ETHERNET_PACKET_FILTER result %i\n", rc);

	printk("Number of endpoints %i\n", interface->descriptor.bNumEndpoints);
	
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