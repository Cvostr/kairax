#include "bootshell_cmdproc.h"
#include "string.h"

#include "dev/device_man.h"

#include "drivers/storage/partitions/storage_partitions.h"

#include "mem/kheap.h"
#include "mem/pmm.h"

#include "dev/acpi/acpi.h"
#include "fs/vfs/vfs.h"
#include "fs/vfs/superblock.h"

#include "proc/process.h"

#include "stdio.h"

#include "proc/syscalls.h"
#include "mod/module_loader.h"
#include "mod/module_stor.h"

#include "proc/thread.h"
#include "kairax/kstdlib.h"

#ifdef X86_64
#include "proc/x64_context.h"
#include "cpu/cpu_local_x64.h"
#endif

void cd(const char* path) 
{         
    sys_set_working_dir(path);
}

uint64_t last_pmm_used_pages = 0;
uint64_t last_total_used_bytes = 0;

void bootshell_process_cmd(char* cmdline) 
{
    int argc = 1;
    int i = 0;
    // подсчет аргументов по количеству пробелов
    for (i = 0; i < strlen(cmdline); i ++) {
        if (cmdline[i] == ' ')
            argc++;
    }

    char* cmdline_temp = cmdline;
    char** args = kmalloc(sizeof(char*) * argc);
    for (i = 0; i < argc; i ++) {
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

    if (strcmp(cmd, "mount") == 0) {

        char* partition_name = args[1];
        char* mnt_path = args[2];

        printf_stdout("Mounting partition %s to path %s\n", partition_name, mnt_path);

        drive_partition_t* partition = get_partition_with_name(partition_name);
        if (partition == NULL) {
            printf_stdout("ERROR: No partition with name %s\n", partition_name);
            goto exit;
        }

        int result = vfs_mount_fs(mnt_path, partition, "ext2");
        if (result < 0) {
            printf_stdout("ERROR: vfs_mount returned with code %i\n", -result);
        }else{
            printf_stdout("Successfully mounted!\n");
        }
    }
    if(strcmp(cmd, "mnroot") == 0) {

        char* partition_name = args[1];

        printf_stdout("Mounting partition %s to root\n", partition_name);

        drive_partition_t* partition = get_partition_with_name(partition_name);
        if (partition == NULL) {
            printf_stdout("ERROR: No partition with name %s\n", partition_name);
            goto exit;
        }

        // mount /
        int result = vfs_mount_fs("/", partition, "ext2");
        if (result < 0) {
            printf_stdout("ERROR: vfs_mount returned with code %i\n", result);
            goto exit;
        } else {
            printf_stdout("Successfully mounted!\n");
        }

        // Монтировать dev
        result = vfs_mount_fs("/dev", NULL, "devfs");

        // Сменить рабочую папку
        cd("/");
    }
    if (strcmp(cmd, "cd") == 0) {
        cd(args[1]);
    }
    if(strcmp(cmd, "unmount") == 0){
        int result = vfs_unmount(args[1]);
    }
    if(strcmp(cmd, "dentry") == 0) {
        dentry_debug_tree();
    }
    if(strcmp(cmd, "inode") == 0) {
        debug_print_inodes(vfs_get_root_dentry()->sb);
    }
    if(strcmp(cmd, "ps") == 0) {
        plist_debug();
    }
    if(strcmp(cmd, "ctx") == 0) {
        pid_t pid = atoi(args[1]);
        struct thread* thr = process_get_by_id(pid);

        if (thr->type != OBJECT_TYPE_THREAD) {
            printk("Not a thread!\n");
            goto exit;
        }

#ifdef X86_64
        thread_frame_t* frame = (thread_frame_t*)thr->context;
        printf("RDI = %s ", ulltoa(frame->rdi, 16));
        printf("RSI = %s ", ulltoa(frame->rsi, 16));
        printk("RIP = %s\n", ulltoa(frame->rip, 16));

        uintptr_t* stack_ptr = (uintptr_t*)frame->rsp;
        int show_stack = 1;
        if (cpu_get_current_vm_table() != NULL) {
            show_stack = vm_is_mapped(cpu_get_current_vm_table(), stack_ptr);
        }
        if (show_stack) {
            printf("\nSTACK TRACE: \n");
            for (int i = 0; i < 25; i ++) {
                uintptr_t value = *(stack_ptr + i);
                printf("%s ", ulltoa(value, 16));
            }
        }
        
        printf("\n");
#endif    
    }
    if(strcmp(cmd, "kill") == 0) {
        pid_t pid = atoi(args[1]);
        int rc = sys_send_signal(pid, SIGKILL);
        if (rc < 0) {
            printf_stdout("Error : %i\n", -rc);
        }
    }
    if(strcmp(cmd, "setuid") == 0) {
        uid_t uid = atoi(args[1]);
        int rc = sys_setgid(uid);
        if (rc < 0) {
            printf_stdout("Error : %i\n", -rc);
        }
        rc = sys_setuid(uid);
        if (rc < 0) {
            printf_stdout("Error : %i\n", -rc);
        }
    }
    if(strcmp(cmd, "insmod") == 0) {
        struct file* mod_file = file_open(NULL, args[1], FILE_OPEN_MODE_READ_ONLY, 0);
        if (mod_file == NULL) {
            goto exit;
        }
        size_t size = mod_file->inode->size;
        char* image_data = kmalloc(size);
        file_read(mod_file, size, image_data);

        int rc = module_load(image_data, size);
        if (rc < 0) {
            printf_stdout("Error loading module : %i\n", -rc);
        }

        file_close(mod_file);
        kfree(image_data);
    }
    if(strcmp(cmd, "unlmod") == 0) {
        int rc = sys_unload_module(args[1]);
        if (rc < 0) {
            printf_stdout("Error unloading module : %i\n", -rc);
        }
    }
    if (strcmp(cmd, "exec") == 0) {

        struct process_create_info info;
        info.current_directory = NULL;
        info.num_args = argc - 1;
        info.args = args + 1;
        info.stdout = 1;
        info.stdin = 0;
        info.stderr = 2;
        pid_t rc = sys_create_process(FD_CWD, args[1], &info);
        if (rc < 0) {
            printf_stdout("Error creating process : %i\n", -rc);
        }

        sys_ioctl(0, 0x5410, rc);

        int status = 0;
        sys_wait(rc, &status, 0);
        
        if (status > 128) {
            int sig = status - 128;
            switch (sig) {
                case SIGABRT:
                    printk("Aborted\n");
                    break;
                case SIGSEGV:
                    printk("Segmentation fault\n");
                    break;
            }
        }
    }
    else if (strcmp(cmdline, "mounts") == 0) {
        struct superblock* sb = NULL;
        int i = 0;
        while ((sb = vfs_get_mounted_sb(i ++)) != NULL) {

            if(sb != NULL) {
                size_t reqd_size = 0;
                char* abs_path_buffer = NULL;
                vfs_dentry_get_absolute_path(sb->root_dir, &reqd_size, NULL);
                abs_path_buffer = kmalloc(reqd_size + 1);
                memset(abs_path_buffer, 0, reqd_size + 1);
                vfs_dentry_get_absolute_path(sb->root_dir, NULL, abs_path_buffer);
                int em = 0;
                printf_stdout("Partition %s mounted to path %s Filesystem %s\n", sb->partition ? sb->partition->name : &em, abs_path_buffer, sb->filesystem->name);

                struct statfs sb_stat;
                int rc = superblock_stat(sb, &sb_stat);
                if (rc == 0)
                    printf_stdout("  Total blocks: %i, free: %i\n", sb_stat.blocks, sb_stat.blocks_free);

                kfree(abs_path_buffer);
            }
        }
    }

    if(strcmp(cmdline, "list partitions") == 0){
        for(int i = 0; i < get_partitions_count(); i ++){
            drive_partition_t* partition = get_partition(i);
            printf_stdout("Partition %s, Start : %i, Size : %i\n", partition->name,
                                                                        partition->start_lba, partition->sectors);
	    }
    }
    if(strcmp(cmdline, "mem") == 0)
    {
        uint64_t pmm_used_pages = pmm_get_used_pages();
        printf_stdout("Physical mem used pages : %i (%i)\n", pmm_used_pages, pmm_used_pages - last_pmm_used_pages);
        last_pmm_used_pages = pmm_used_pages;

        uint64_t total_free_bytes = 0;
        uint64_t total_used_bytes = 0;
        kheap_item_t* current_item = kheap_get_head_item();
        while(current_item != NULL){
            if(current_item->free)
                total_free_bytes += current_item->size;
            else
                total_used_bytes += current_item->size;
            //printf_stdout("kheap item Addr : %i, Size : %i, Free : %i\n", 
            //    V2P(current_item), current_item->size, current_item->free);
            current_item = current_item->next;
        }
        printf_stdout("Kheap free space: %i\n", total_free_bytes);
        printf_stdout("Kheap used space: %i (%i)\n", total_used_bytes, total_used_bytes - last_total_used_bytes);

        last_total_used_bytes = total_used_bytes;
    }
    if (strcmp(cmdline, "acpi") == 0) {
        printf_stdout("ACPI OEM = %s\n", acpi_get_oem_str());
        printf_stdout("ACPI version = %i\n", acpi_get_revision());
        for(uint32_t i = 0; i < acpi_get_cpus_apic_count(); i ++){
		    printf_stdout("APIC on CPU %i Id : %i \n", acpi_get_cpus_apic()[i]->acpi_cpu_id, acpi_get_cpus_apic()[i]->lapic_id);
	    }
    }
    else if(strcmp(cmd, "shutdown") == 0){
        sys_poweroff(0x30);
    }
    else if(strcmp(cmd, "reboot") == 0){
        sys_poweroff(0x40);
    }
    if(strcmp(cmdline, "dev") == 0){

        struct device* dev = NULL;
        int i = 0;
        while ((dev = get_device(i ++)) != NULL) {

            if(dev != NULL) {
                printf_stdout("Device %s.\n", dev->dev_name);
                //printf_stdout("\tID: %i-%i-%i-%i-%i\n", dev->id.d1, dev->id.d2, dev->id.d3, dev->id.d4, dev->id.d5);
                if (dev->dev_bus == DEVICE_BUS_PCI) {
                    struct pci_device_info* desc = dev->pci_info;
                    printf_stdout("\tPCI: %i:%i:%i class %i, subclass %i, pif %i\n", desc->bus, desc->device, desc->function, desc->device_class, desc->device_subclass, desc->prog_if);
                }
                if (dev->dev_bus == DEVICE_BUS_USB) {
                    struct usb_device* usbdev = dev->usb_info.usb_device;
                    struct usb_device_descriptor* devdescr = &usbdev->descriptor;
                    printf_stdout("\tUSB: class %i, subclass %i, vendor %x product %x\n", devdescr->bDeviceClass, devdescr->bDeviceSubClass, devdescr->idVendor, devdescr->idProduct);
                }
                if (dev->dev_type == DEVICE_TYPE_NETWORK_ADAPTER) {
                    
                }
            }
        }
    }

exit:
    for (int i = 0; i < argc; i ++) {
        kfree(args[i]);
    }
    kfree(args);
}