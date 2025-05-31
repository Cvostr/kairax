#include "dev/device_man.h"

struct usb_device_id usb_mass_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .bInterfaceClass = 0x08,
        .bInterfaceProtocol = 0x50
    },
	{0,}
};

int usb_mass_device_probe(struct device *dev) 
{
	struct usb_interface* interface = dev->usb_info.usb_interface;
    struct usb_device* device = dev->usb_info.usb_device;
    printk("USB mass storage %x %x\n", interface->descriptor.bInterfaceClass, interface->descriptor.bInterfaceProtocol);

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
    printk("USB Mass: reset result %i\n", rc);

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
