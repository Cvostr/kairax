#include "device_drivers.h"
#include "list/list.h"
#include "device_man.h"

list_t pci_dev_drivers = {0,};
list_t usb_dev_drivers = {0,};

int register_pci_device_driver(struct pci_device_driver* driver)
{
    list_add(&pci_dev_drivers, driver);

    probe_devices();
}

int register_usb_device_driver(struct usb_device_driver* driver)
{
    list_add(&usb_dev_drivers, driver);

    probe_devices();
}

int is_pci_id_term(struct pci_device_id* pci_dev_id) 
{
    if (pci_dev_id->vendor_id == 0 && pci_dev_id->dev_id == 0) 
    {
        return TRUE;
    }

    return FALSE;
}

int pci_id_terms_equals(struct pci_device_id* req_pci_dev_id, struct pci_device_id* drv_pci_dev_id) 
{
    int vendor_id_eq = drv_pci_dev_id->vendor_id == PCI_ANY_ID ? TRUE : (req_pci_dev_id->vendor_id == drv_pci_dev_id->vendor_id);
    int device_id_eq = drv_pci_dev_id->dev_id == PCI_ANY_ID ? TRUE : (req_pci_dev_id->dev_id == drv_pci_dev_id->dev_id);
    int device_class_eq = drv_pci_dev_id->dev_class == 0 ? TRUE : (req_pci_dev_id->dev_class == drv_pci_dev_id->dev_class);
    int device_subclass_eq = drv_pci_dev_id->dev_subclass == 0 ? TRUE : (req_pci_dev_id->dev_subclass == drv_pci_dev_id->dev_subclass);
    int device_pif_eq = drv_pci_dev_id->dev_pif == 0 ? TRUE : (req_pci_dev_id->dev_pif == drv_pci_dev_id->dev_pif);

    return (vendor_id_eq && device_id_eq && device_class_eq && device_subclass_eq && device_pif_eq); 
}

struct pci_device_driver* drivers_get_for_pci_device(struct pci_device_id* pci_dev_id)
{
    struct list_node* current_node = pci_dev_drivers.head;
    struct pci_device_driver* driver = NULL;

    for (size_t i = 0; i < pci_dev_drivers.size; i++) {
        driver = (struct pci_device_driver*) current_node->element;
        struct pci_device_id* driver_id = driver->pci_device_ids;
        
        while (is_pci_id_term(driver_id) == FALSE) {

            if (pci_id_terms_equals(pci_dev_id, driver_id)) {
                return driver;
            }

            driver_id ++;
        }

        // Переход на следующий элемент
        current_node = current_node->next;
    }

    return NULL;
}



int is_usb_id_term(struct usb_device_id* usb_dev_id) 
{
    if (usb_dev_id->match_flags == 0)
    {
        return TRUE;
    }

    return FALSE;
}

int usb_device_id_equals(struct usb_device_id* req_usb_dev_id, struct usb_device_id* drv_usb_dev_id) 
{
    // Проверка idVendor
    if (((drv_usb_dev_id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) == USB_DEVICE_ID_MATCH_VENDOR) && (req_usb_dev_id->idVendor != drv_usb_dev_id->idVendor))
    {
        return FALSE;
    }

    // Проверка idProduct
    if (((drv_usb_dev_id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) == USB_DEVICE_ID_MATCH_PRODUCT) && (req_usb_dev_id->idProduct != drv_usb_dev_id->idProduct))
    {
        return FALSE;
    }

    // Проверка bcdDevice
    if (((drv_usb_dev_id->match_flags & USB_DEVICE_ID_MATCH_BCDDEVICE) == USB_DEVICE_ID_MATCH_BCDDEVICE) && (req_usb_dev_id->bcdDevice != drv_usb_dev_id->bcdDevice))
    {
        return FALSE;
    }

    // Проверка bDeviceClass
    if (((drv_usb_dev_id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) == USB_DEVICE_ID_MATCH_DEV_CLASS) && (req_usb_dev_id->bDeviceClass != drv_usb_dev_id->bDeviceClass))
    {
        return FALSE;
    }

    // Проверка bDeviceSubclass
    if (((drv_usb_dev_id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) == USB_DEVICE_ID_MATCH_DEV_SUBCLASS) && (req_usb_dev_id->bDeviceSubclass != drv_usb_dev_id->bDeviceSubclass))
    {
        return FALSE;
    }

    // Проверка bDeviceProtocol
    if (((drv_usb_dev_id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) == USB_DEVICE_ID_MATCH_DEV_PROTOCOL) && (req_usb_dev_id->bDeviceProtocol != drv_usb_dev_id->bDeviceProtocol))
    {
        return FALSE;
    }

    // Проверка bInterfaceClass
    if (((drv_usb_dev_id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) == USB_DEVICE_ID_MATCH_INT_CLASS) && (req_usb_dev_id->bInterfaceClass != drv_usb_dev_id->bInterfaceClass))
    {
        return FALSE;
    }

    // Проверка bInterfaceSubclass
    if (((drv_usb_dev_id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) == USB_DEVICE_ID_MATCH_INT_SUBCLASS) && (req_usb_dev_id->bInterfaceSubclass != drv_usb_dev_id->bInterfaceSubclass))
    {
        return FALSE;
    }

    // Проверка bIntefaceProtocol
    if (((drv_usb_dev_id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) == USB_DEVICE_ID_MATCH_INT_PROTOCOL) && (req_usb_dev_id->bInterfaceProtocol != drv_usb_dev_id->bInterfaceProtocol))
    {
        return FALSE;
    }

    return TRUE;
}

struct usb_device_driver* drivers_get_for_usb_device(struct usb_device_id* usb_dev_id)
{
    struct list_node* current_node = usb_dev_drivers.head;
    struct usb_device_driver* driver = NULL;

    for (size_t i = 0; i < usb_dev_drivers.size; i++) 
    {
        driver = (struct usb_device_driver*) current_node->element;
        struct usb_device_id* driver_id = driver->device_ids;
        
        while (is_usb_id_term(driver_id) == FALSE) 
        {
            if (usb_device_id_equals(usb_dev_id, driver_id)) 
            {
                return driver;
            }

            driver_id ++;
        }

        // Переход на следующий элемент
        current_node = current_node->next;
    }

    return NULL;
}