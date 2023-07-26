#include "bootshell_cmdproc.h"
#include "string.h"

#include "bus/pci/pci.h"

#include "drivers/storage/devices/storage_devices.h"
#include "drivers/storage/partitions/storage_partitions.h"

#include "mem/kheap.h"
#include "mem/pmm.h"

#include "dev/acpi/acpi.h"
#include "fs/vfs/vfs.h"
#include "fs/vfs/superblock.h"

#include "proc/process.h"

#include "stdio.h"

extern char curdir[512];
extern struct inode* wd_inode;
extern struct dentry* wd_dentry;

void bootshell_process_cmd(char* cmdline) 
{
    int argc = 1;
    // подсчет аргументов по количеству пробелов
    for (int i = 0; i < strlen(cmdline); i ++) {
        if (cmdline[i] == ' ')
            argc++;
    }

    char* cmdline_temp = cmdline;
    char** args = kmalloc(sizeof(char*) * argc);
    for (int i = 0; i < argc; i ++) {
        char* space_ptr = strchr(cmdline_temp, ' ');
        int len = 0;
        if (space_ptr != NULL) {
            len = space_ptr - cmdline_temp;
        } else {
            len = strlen(cmdline_temp);
        }
        
        args[i] = kmalloc(sizeof(char) * (len + 1));
        strncpy(args[i], cmdline_temp, len);
        cmdline_temp = space_ptr + 1;
    }

    char* cmd = args[0];

    if(strcmp(cmd, "mount") == 0){

        char* partition_name = args[1];
        char* mnt_path = args[2];

        printf("Mounting partition %s to path %s\n", partition_name, mnt_path);

        drive_partition_t* partition = get_partition_with_name(partition_name);
        if(partition == NULL){
            printf("ERROR: No partition with name %s\n", partition_name);
            goto exit;
        }

        int result = vfs_mount(mnt_path, partition);
        if(result < 0){
            printf("ERROR: vfs_mount returned with code %i\n", result);
        }else{
            printf("Successfully mounted!\n");
        }

    }
    if(strcmp(cmd, "cd") == 0){
        if (!wd_inode) {
            inode_close(wd_inode);
        }
        struct dentry* new_dentry;
        wd_inode = vfs_fopen(wd_dentry, args[1], 0, &new_dentry);
        if (wd_inode == NULL) {
            printf("ERROR: cant cd to %s\n", args[1]);
            goto exit;
        }
        wd_dentry = new_dentry;
        memset(curdir, 0, 512);
        dentry_get_absolute_path(wd_dentry, NULL, curdir);
                
    }
    if(strcmp(cmd, "unmount") == 0){
        int result = vfs_unmount(args[1]);
    }
    if(strcmp(cmd, "path") == 0){
        int offset = 0;
        struct superblock* result = vfs_dentry_traverse_path(vfs_get_root_dentry(), args[1])->sb;
        if (result == NULL) {
            printf("No mounted device\n");
        }else 
            printf("Partition: %s\n", result->partition->name);
        
    }
    if(strcmp(cmd, "ls") == 0) {
        uint32_t index = 0;

        struct inode* inode = vfs_fopen(wd_dentry, argc > 1 ? args[1] : NULL, 0, NULL);
        if(inode == NULL){
            printf("Can't open directory with path : ", args);
            goto exit;
        }
        struct dirent* child = NULL;
        while((child = inode_readdir(inode, index++)) != NULL){
            //printf("TYPE %s, NAME %s, SIZE %i\n", (child->type == DT_REG) ? "FILE" : "DIR", child->name, child->size);
            printf("TYPE %s,   NAME %s   INODE %i\n", (child->type == DT_REG) ? "FILE" : "DIR", child->name, child->inode);
            kfree(child);
        }

        inode_close(inode); 
    }
    if(strcmp(cmd, "chmod") == 0) {
        struct inode* inode = vfs_fopen(wd_dentry, args[1], 0, NULL);
        if(inode == NULL){
            printf("Can't open file with path : %s", args[1]);
            goto exit;
        }

        inode_chmod(inode, 0xFFF);
        inode_close(inode);
    }
    if(strcmp(cmd, "mkdir") == 0) {
        struct inode* inode = vfs_fopen(NULL, "/", 0, NULL);
        if(inode == NULL){
            printf("Can't open file with path : %s", args[1]);
            goto exit;
        }

        int rc = inode_mkdir(inode, args[1], 0xFFF);
        if (rc != 0) {
            printf("Error creating dir : %i", rc);
        }
        inode_close(inode);
    }
    if(strcmp(cmd, "cat") == 0){
        struct inode* inode = vfs_fopen(wd_dentry, args[1], 0, NULL);
        if(inode == NULL){
            printf("Can't open directory with path : %s", args[1]);
            goto exit;
        }
        int size = inode->size;
        printf("%s: ", args[1]);
        char* buffer = kmalloc(size);

        loff_t offset = 0;
        inode_read(inode, &offset, size, buffer);
        for(int i = 0; i < size; i++){
            printf("%c", buffer[i]);
        }
        printf("\n");
        kfree(buffer);
        inode_close(inode);
    }
    if(strcmp(cmd, "stress") == 0) {
        struct inode* sysn_i = vfs_fopen(NULL, "/sysn.a", 0, NULL);
        struct inode* sysc_i = vfs_fopen(NULL, "/sysc.a", 0, NULL);
        struct inode* ls_i = vfs_fopen(NULL, "/ls.a", 0, NULL);

        loff_t offset = 0;
        int size = sysn_i->size;
        char* sysn_d = kmalloc(size);

        inode_read(sysn_i, &offset, size, sysn_d);

        size = sysc_i->size;
        char* sysc_d = kmalloc(size);
        offset = 0;
        inode_read(sysc_i, &offset, size, sysc_d);

        size = ls_i->size;
        char* ls_d = kmalloc(size);
        offset = 0;
        inode_read(ls_i, &offset, size, ls_d);

        for (int i = 0; i < 30; i ++) {
            int rc = create_new_process_from_image(NULL, sysn_d); 
            rc = create_new_process_from_image(NULL, sysc_d); 
            rc = create_new_process_from_image(NULL, ls_d); 
        }

        kfree(sysn_d);
        kfree(sysc_d);
        kfree(ls_d);
        
        inode_close(sysn_i);
        inode_close(sysc_i);
        inode_close(ls_i);
    }
    if(strcmp(cmd, "exec") == 0) {
        struct inode* inode = vfs_fopen(NULL, args[1], 0, NULL);
        if(inode == NULL){
            printf("Can't open file with path : ", args[1]);
            return;
        }
        int size = inode->size;
        printf("%s: ", args);
        char* buffer = kmalloc(size);
        if(buffer == NULL) {
            printf("Error allocating memory");
            return;
        }

        loff_t offset = 0;
        inode_read(inode, &offset, size, buffer);

        //Запуск
        int rc = create_new_process_from_image(NULL, buffer); 

        kfree(buffer);
        inode_close(inode);
    }
    else if(strcmp(cmdline, "mounts") == 0){
        struct superblock** mounts = vfs_get_mounts();
        for(int i = 0; i < 100; i ++){
            struct superblock* mount = mounts[i];
            if(mount != NULL) {
                size_t reqd_size = 0;
                char* abs_path_buffer = NULL;
                dentry_get_absolute_path(mount->root_dir, &reqd_size, NULL);
                abs_path_buffer = kmalloc(reqd_size + 1);
                memset(abs_path_buffer, 0, reqd_size + 1);
                dentry_get_absolute_path(mount->root_dir, NULL, abs_path_buffer);
                printf("Partition %s mounted to path %s/ Filesystem %s\n", mount->partition->name, abs_path_buffer, mount->filesystem->name);
                kfree(abs_path_buffer);
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
            //printf("kheap item Addr : %i, Size : %i, Free : %i\n", 
            //    V2P(current_item), current_item->size, current_item->free);
            current_item = current_item->next;
        }
        printf("Kheap free space: %i\n", total_free_bytes);
        printf("Kheap used space: %i\n", total_used_bytes);
    }
    if(strcmp(cmdline, "acpi") == 0){
        printf("ACPI OEM = %s\n", acpi_get_oem_str());
        printf("ACPI version = %i\n", acpi_get_revision());
        for(uint32_t i = 0; i < acpi_get_cpus_apic_count(); i ++){
		    printf("APIC on CPU %i Id : %i \n", acpi_get_cpus_apic()[i]->acpi_cpu_id, acpi_get_cpus_apic()[i]->lapic_id);
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

exit:
    for (int i = 0; i < argc; i ++) {
        kfree(args[i]);
    }
    kfree(args);
}