#include "device_drivers.h"
#include "list/list.h"
#include "device_man.h"

list_t pci_dev_drivers = {0};

int register_pci_device_driver(struct pci_device_driver* driver)
{
    list_add(&pci_dev_drivers, driver);

    probe_devices();
}

int is_pci_id_term(struct pci_device_id* pci_dev_id) 
{
    if (pci_dev_id->vendor_id == 0 && pci_dev_id->dev_id == 0) {
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