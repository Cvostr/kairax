#include "bootshell_cmdproc.h"
#include "string.h"

#include "bus/pci/pci.h"

#include "drivers/storage/devices/storage_devices.h"
#include "drivers/storage/partitions/storage_partitions.h"

#include "mem/kheap.h"
#include "mem/pmm.h"

#include "dev/acpi/acpi.h"
#include "fs/vfs/vfs.h"

#include "proc/process.h"

#include "stdio.h"

void bootshell_process_cmd(char* cmdline){
    char* cmd = NULL;
    char* args = NULL;
    char* space_index = strchr(cmdline, ' ');
    if(space_index != NULL){
        int cmdlen = space_index - cmdline;
        cmd = kmalloc(cmdlen + 1);
        strncpy(cmd, cmdline, cmdlen);
        args = space_index + 1;
    } else {
        int cmdline_len = strlen(cmdline);
        cmd = kmalloc(cmdline_len + 1);
        strncpy(cmd, cmdline, cmdline_len);
    }

    if(strcmp(cmd, "mount") == 0){
        char* args_space_index = strchr(args, ' ');
        int partition_name_len = args_space_index - args;
        int mount_path_len = strlen(args_space_index + 1);

        char* partition_name = kmalloc(partition_name_len + 1);
        strncpy(partition_name, args, partition_name_len);

        char* mnt_path = kmalloc(mount_path_len + 1);
        strcpy(mnt_path, args_space_index + 1);

        printf("Mounting partition %s to path %s\n", partition_name, mnt_path);

        drive_partition_t* partition = get_partition_with_name(partition_name);
        if(partition == NULL){
            printf("ERROR: No partition with name %s\n", partition_name);
            kfree(partition_name);
            kfree(mnt_path);
            return;
        }

        int result = vfs_mount(mnt_path, partition);
        if(result < 0){
            printf("ERROR: vfs_mount returned with code %i\n", result);
        }else{
            printf("Successfully mounted!\n");
        }

        kfree(partition_name);
        kfree(mnt_path);
    }
    if(strcmp(cmd, "unmount") == 0){
        int result = vfs_unmount(args);
    }
    if(strcmp(cmd, "path") == 0){
        int offset = 0;
        vfs_mount_info_t* result = vfs_get_mounted_partition_split(args, &offset);
        if(result == NULL){
            printf("No mounted device\n");
        }else 
            printf("Partition: %s, Subpath: %s\n", result->partition->name, args + offset);
        
    }
    if(strcmp(cmd, "ls") == 0){
        uint32_t index = 0;
        vfs_inode_t* inode = vfs_fopen(args, 0);
        if(inode == NULL){
            printf("Can't open directory with path : ", args);
            return;
        }
        vfs_inode_t* child = NULL;
        while((child = vfs_readdir(inode, index++)) != NULL){
            printf("TYPE %s, NAME %s, SIZE %i\n", (child->flags == VFS_FLAG_FILE) ? "FILE" : "DIR", child->name, child->size);
            kfree(child);
        }

        vfs_close(inode);
        kfree(inode);     
    }
    if(strcmp(cmd, "cat") == 0){
        vfs_inode_t* inode = vfs_fopen(args, 0);
        if(inode == NULL){
            printf("Can't open directory with path : ", args);
            return;
        }
        int size = inode->size;
        printf("%s: ", args);
        char* buffer = kmalloc(size);
        vfs_read(inode, 0, size, buffer);
        for(int i = 0; i < size; i++){
            printf("%c", buffer[i]);
        }
        printf("\n");
        kfree(buffer);
        vfs_close(inode);
        kfree(inode);
    }
    if(strcmp(cmd, "exec") == 0) {
        vfs_inode_t* inode = vfs_fopen(args, 0);
        if(inode == NULL){
            printf("Can't open directory with path : ", args);
            return;
        }
        int size = inode->size;
        printf("%s: ", args);
        char* buffer = kmalloc(size);
        if(buffer == NULL) {
            printf("Error allocating memory");
            return;
        }

        vfs_read(inode, 0, size, buffer);

        //Запуск
        process_t* proc = create_new_process_from_image(buffer); 

        kfree(buffer);
        vfs_close(inode);
        kfree(inode);
    }
    else if(strcmp(cmdline, "mounts") == 0){
        vfs_mount_info_t** mounts = vfs_get_mounts();
        for(int i = 0; i < 100; i ++){
            vfs_mount_info_t* mount = mounts[i];
            if(mount != NULL){
                printf("Partition %s mounted to path %s/ Filesystem %s\n", mount->partition->name, mount->mount_path, mount->filesystem->name);
            }
        }
    }
    else if(strcmp(cmdline, "list disks") == 0){
        for(int i = 0; i < get_drive_devices_count(); i ++){
            drive_device_t* device = get_drive(i);
            printf("Drive Name %s, Model %s, Serial %s, Size : %i MiB\n", device->name, device->model, device->serial, device->bytes / (1024UL * 1024));
        }
    }
    if(strcmp(cmdline, "list partitions") == 0){
        for(int i = 0; i < get_partitions_count(); i ++){
            drive_partition_t* partition = get_partition(i);
            printf("Partition Name %s, Index : %i, Start : %i, Size : %i\n", partition->name, partition->index,
                                                                        partition->start_lba, partition->sectors);
	    }
    }
    if(strcmp(cmdline, "mem") == 0){
        printf("Physical mem used pages : %i\n", pmm_get_used_pages());

        uint64_t total_free_bytes = 0;
        uint64_t total_used_bytes = 0;
        kheap_item_t* current_item = kheap_get_head_item();
        while(current_item != NULL){
            if(current_item->free)
                total_free_bytes += current_item->size;
            else
                total_used_bytes += current_item->size;
            printf("kheap item Addr : %i, Size : %i, Free : %i, Prev %i\n", 
                V2P(current_item), current_item->size, current_item->free, V2P(current_item->prev));
            current_item = current_item->next;
        }
        printf("Kheap free space: %i\n", total_free_bytes);
        printf("Kheap used space: %i\n", total_used_bytes);
    }
    if(strcmp(cmdline, "acpi") == 0){
        printf("ACPI OEM = %s\n", acpi_get_oem_str());
        printf("ACPI version = %i\n", acpi_get_revision());
        for(uint32_t i = 0; i < acpi_get_cpus_apic_count(); i ++){
		    printf("APIC on CPU %i Id : %i \n", acpi_get_cpus_apic()[i]->acpi_cpu_id, acpi_get_cpus_apic()[i]->apic_id);
	    }
    }
    else if(strcmp(cmdline, "shutdown") == 0){
        acpi_poweroff();
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
                case 0x8: printf("Mass Storage NVME controller"); 
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

    if(cmd != NULL){
        kfree(cmd);
    }
}