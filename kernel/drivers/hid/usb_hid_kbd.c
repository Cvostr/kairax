#include "dev/device_man.h"

struct usb_device_id usb_hid_kbd_ids[] = {
	{   
        .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .bInterfaceClass = 3,
        .bInterfaceSubclass = 1,
        .bInterfaceProtocol = 1
    },
	{0,}
};

int usb_hid_kbd_device_probe(struct device *dev) 
{
	return 0;
}

struct device_driver_ops usb_hid_kbd_ops = {

    .probe = usb_hid_kbd_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct usb_device_driver usb_hid_kbd_storage_driver = {
	.device_ids = usb_hid_kbd_ids,
	.ops = &usb_hid_kbd_ops
};

void usb_hid_kbd_init()
{
	register_usb_device_driver(&usb_hid_kbd_storage_driver);

}
