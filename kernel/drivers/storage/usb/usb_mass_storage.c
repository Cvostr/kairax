#include "dev/device_man.h"

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

#define CBW_SIGNATURE  0x43425355

#define SCSI_READ_10 0x28

#define CBW_TO_DEVICE	0x00
#define CBW_TO_HOST		0x80

#define CSW_SUCCESS		0x00
#define CSW_FAILED		0x01
#define CSW_PHASE_ERROR	0x02

struct usb_device_id usb_mass_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .bInterfaceClass = 0x08,
		//.bInterfaceSubclass = 0x06,
        .bInterfaceProtocol = 0x50
    },
	{0,}
};

void usb_mass_read(struct usb_device* device, struct usb_endpoint* ep, uint8_t lun, uint64_t sector, uint16_t count)
{
	struct command_block_wrapper cbw;
	cbw.signature = CBW_SIGNATURE;
	cbw.tag = 0;
    cbw.length = 512 * count;
    cbw.flags = CBW_TO_HOST;
    cbw.command_len = 10;
	cbw.lun = lun;

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
    cbw.data[7] = (uint8_t) ((count >> 8) & 0xFF);
    cbw.data[8] = (uint8_t) ((count) & 0xFF);

	xhci_device_send_bulk_data(device->controller_device_data, ep, &cbw, sizeof(cbw));
}

int usb_mass_device_probe(struct device *dev) 
{
	struct usb_interface* interface = dev->usb_info.usb_interface;
    struct usb_device* device = dev->usb_info.usb_device;
    //printk("USB mass storage %x %x\n", interface->descriptor.bInterfaceClass, interface->descriptor.bInterfaceProtocol);

    int rc;

    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = 0xFF;
	req.wValue = 0;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = 0; // Данный вид запроса не имеет выходных данных

    rc = usb_device_send_request(device, &req, NULL, 0);
    //printk("USB Mass: reset result %i\n", rc);
	if (rc != 0)
	{
		printk("USB: Reset failed (%i)\n", rc);
		return -1;
	}

    uint8_t lun = 0;

    struct usb_device_request lun_req;
	lun_req.type = USB_DEVICE_REQ_TYPE_CLASS;
	lun_req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	lun_req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	lun_req.bRequest = 0xFE;
	lun_req.wValue = 0;
	lun_req.wIndex = interface->descriptor.bInterfaceNumber;
	lun_req.wLength = 1;

    rc = usb_device_send_request(device, &req, &lun, 1);
    printk("USB Mass: lun req result %i, lun = %i\n", rc, lun);

	struct usb_endpoint* builk_in = NULL;
	struct usb_endpoint* builk_out = NULL;

	for (uint8_t i = 0; i < interface->descriptor.bNumEndpoints; i ++)
	{
		struct usb_endpoint* ep = &interface->endpoints[i];

		if (ep->descriptor.bmAttributes != USB_ENDPOINT_ATTR_TT_BULK)
			continue;

		if ((ep->descriptor.bEndpointAddress & USB_ENDPOINT_ADDR_DIRECTION_IN) == USB_ENDPOINT_ADDR_DIRECTION_IN)
		{
			// IN
			printk("USB: Mass storage IN %i\n", i);
			builk_in = ep;
		}
		else 
		{
			// OUT
			printk("USB: Mass storage OUT %i\n", i);
			builk_out = ep;
		}
	}

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

	//printk("1");
	// Считать 3 сектора
	usb_mass_read(device, builk_out, 0, 0, 3);
	//printk("2");
	usb_mass_read(device, builk_out, 0, 0, 5);

	//for (int i = 0; i < 100; i ++)
	//{
	//
	//}
}

struct device_driver_ops usb_mass_ops = {

    .probe = usb_mass_device_probe
    //void (*remove) (struct device *dev);
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
