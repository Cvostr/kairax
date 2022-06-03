#include "bootshell_cmdproc.h"
#include "string.h"

#include "bus/pci/pci.h"

#include "drivers/storage/devices/storage_devices.h"
#include "drivers/storage/partitions/storage_partitions.h"

#include "mem/kheap.h"
#include "mem/pmm.h"

#include "dev/acpi/acpi.h"

void bootshell_process_cmd(char* cmdline){
    if(strcmp(cmdline, "list disks") == 0){
        for(int i = 0; i < get_drive_devices_count(); i ++){
            drive_device_header_t* device = get_drive(i);
            printf("Drive Name %s, Model %s, Size : %i MiB\n", device->name, device->model, device->bytes / (1024UL * 1024));
        }
    }
    if(strcmp(cmdline, "list partitions") == 0){
        for(int i = 0; i < get_partitions_count(); i ++){
            drive_partition_header_t* partition = get_partition(i);
            printf("Partition Name %s, Index : %i, Start : %i, Size : %i\n", partition->name, partition->index,
                                                                        partition->start_sector, partition->sectors);
	    }
    }
    if(strcmp(cmdline, "mem") == 0){
        printf("Physical mem used pages : %i\n", pmm_get_used_pages());
        /*kheap_item_t* current_item = kheap_get_head_item();
        while(current_item != NULL){
            printf("kheap item Addr : %i, Size : %i, Free : %i\n", current_item, current_item->size, current_item->free);
            current_item = current_item->next;
        }*/
    }
    if(strcmp(cmdline, "acpi") == 0){
        printf("ACPI OEM = %s\n", acpi_get_oem_str());
        printf("ACPI version = %i\n", acpi_get_revision());
    }
    if(strcmp(cmdline, "pci") == 0){
        printf("PCI devices %i \n", get_pci_devices_count());
        for(int i = 0; i < get_pci_devices_count(); i ++){
            pci_device_desc* desc = &get_pci_devices_descs()[i];
            printf("%i:%i:%i ", desc->bus, desc->device, desc->function);
            
            switch(desc->device_class){
            case 0x1: {//Mass storage
                switch(desc->device_subclass){ //IDE contrl
                case 0x1: printf("Mass Storage IDE Controller"); 
                    break;
                case 0x6: printf("Mass Storage SATA"); 
                    if(desc->prog_if == 0x1){
                        printf(" AHCI controller");
                    }else{
                        printf(" controller");
                    } 
                    break;
                }
            break;
            }
            case 0x2: {//Internet
            switch(desc->device_subclass){
                case 0x0: printf("Network Ethernet controller"); break;  //Ethernet
                }
            break;
            }

            case 0x3:{ //Graphics
            switch(desc->device_subclass){ 
                case 0x0: printf("VGA graphics controller"); break;//VGA
                case 0x02: printf("3D graphics controller"); break;
                case 0x80: printf("Unknown graphics card"); break;
                }
            break;
            }

            case 0x4:{ //Multimedia
            switch(desc->device_subclass){ 
                case 0x03: printf("Multimedia Audio device"); break;
                }
            break;
            }

            case 0x5:{ //Multimedia
            switch(desc->device_subclass){ 
                case 0x80: printf("Unknown Memory controller"); break;
                }
            break;
            }
            case 0x6:{ //System bridge devices
            switch(desc->device_subclass){ //
                case 0x0: printf("System host bridge"); break;
                case 0x1: printf("System ISA bridge"); break;
                case 0x4: printf("System PCI-PCI bridge"); break;
                case 0x80: printf("System unknown bridge"); break;
                }
            break;
            }

            case 0x7:{ //Simple communication port devices
            switch(desc->device_subclass){ //
                case 0x0: printf("System host bridge"); break;
                case 0x1: printf("System ISA bridge"); break;
                case 0x4: printf("System PCI-PCI bridge"); break;
                case 0x80: printf("Unknown Simple Communications dev"); break;
                }
            break;
            }

            case 0xC:{ //Serial port devices
            switch(desc->device_subclass){ //
                case 0x3: printf("USB Controller"); break;
                case 0x5: printf("SMBus"); break;
                }
            break;
            }
            default:{
                printf("Unknown");
                printf("%i %i %i %i %i\n", desc->vendor_id, desc->device_id, desc->device_class, desc->device_subclass);
                break;
            }
        }
        printf("\n");

        }
    }
}