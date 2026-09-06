#include "dev/device_man.h"
#include "kairax/stdio.h"
#include "mem/iomem.h"
#include "kairax/string.h"

#define USBHUB_IFACE_PROTOCOL_SINGLE_TT     1
#define USBHUB_IFACE_PROTOCOL_MULTI_TT      2
#define USBHUB_IFACE_PROTOCOL_SUPER_SPEED   3

// ДРАЙВЕР НЕ ЗАВЕРШЕН
// СКОРЕЕ ДАЖЕ ТОЛЬКО НАЧАТ

int usb_hub_probe(struct device *dev) 
{
    struct usb_interface* interface = dev->usb_info.usb_interface;
    struct usb_device* device = dev->usb_info.usb_device;

    int bInterfaceProtocol = interface->descriptor.bInterfaceProtocol;

    printk("USB HUB DEVICE. bInterfaceProtocol = %i\n", bInterfaceProtocol);

    return 0;
}

void usb_hub_remove(struct device *dev) 
{
}

struct device_driver_ops usb_hub_ops = {

    .probe = usb_hub_probe,
    .remove = usb_hub_remove
    //void (*shutdown) (struct device *dev);
};

struct usb_device_id usb_hub_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS,
        .bInterfaceClass = 0x09,
    },
	{0,}
};

struct usb_device_driver usb_hub_driver = {
	.device_ids = usb_hub_ids,
	.ops = &usb_hub_ops
};

int usb_hub_init(void)
{
	register_usb_device_driver(&usb_hub_driver);
	return 0;
}

void usb_mass_deinit(void)
{

}