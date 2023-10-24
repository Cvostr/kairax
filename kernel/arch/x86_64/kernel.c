#include "drivers/video/video.h"
#include "stdio.h"
#include "kstdlib.h"
#include "interrupts/idt.h"
#include "interrupts/pic.h"
#include "interrupts/handle/handler.h"
#include "dev/keyboard/int_keyboard.h"
#include "bus/pci/pci.h"

#include "drivers/storage/ahci/ahci.h"
#include "drivers/storage/nvme/nvme.h"

#include "memory/mem_layout.h"
#include "mem/pmm.h"
#include "memory/kernel_vmm.h"
#include "mem/kheap.h"

#include "boot/multiboot.h"
#include "string.h"

#include "proc/thread_scheduler.h"
#include "dev/cmos/cmos.h"
#include "dev/acpi/acpi.h"
#include "drivers/storage/devices/storage_devices.h"
#include "drivers/storage/partitions/storage_partitions.h"

#include "fs/vfs/vfs.h"
#include "fs/ext2/ext2.h"
#include "fs/devfs/devfs.h"

#include "misc/bootshell/bootshell.h"
#include "cpu/gdt.h"
#include "cpu/msr.h"
#include "cpu/smp.h"
#include "interrupts/apic.h"
#include "cpu/cpuid.h"
#include "time/timer.h"

void kmain(uint32_t multiboot_magic, void* multiboot_struct_ptr){
	parse_mb2_tags(multiboot_struct_ptr);

	kernel_boot_info_t* kboot_info = get_kernel_boot_info();

	init_pmm();

	pmm_params_t pmm_params;
	memset(&pmm_params, 0, sizeof(pmm_params_t));
	pmm_params.kernel_base = kboot_info->load_base_addr;
	// Проходимся по регионам grub
	for (int i = 0; i < kboot_info->mmap_len; i ++) {
		uintptr_t start, length;
		uint32_t type;
		multiboot_get_memory_area(i, &start, &length, &type);
		uintptr_t end = start + length;
		
		if (end > pmm_params.physical_mem_max_addr)
			pmm_params.physical_mem_max_addr = end;
		
		//Считаем физическую память
		pmm_params.physical_mem_total += length;

		if (type != 1)
			pmm_set_mem_region(start, length);

		//printf("REGION : %i %i %i\n", start, end, type);
	}

	pmm_take_base_regions();
	pmm_set_params(&pmm_params);

	init_pic();
	setup_idt();
	
	page_table_t* new_pt = create_kernel_vm_map();
	vmm_use_kernel_vm();

	vga_init(P2V(kboot_info->fb_info.fb_addr), kboot_info->fb_info.fb_pitch, kboot_info->fb_info.fb_width, kboot_info->fb_info.fb_height,
	kboot_info->fb_info.fb_bpp);
	printf("Kairax Kernel v0.1\n");
	//printf("LOADER : %s\n", kboot_info->bootloader_string);
	//printf("CMDLINE : %s\n", P2V(kboot_info->command_line));
	//printf("width %i, height %i, addr %i\n", kboot_info->fb_info.fb_width, kboot_info->fb_info.fb_height, kboot_info->fb_info.fb_addr);

	int rc = 0;
	printf("KHEAP: Initialization...\n");
	if ((rc = kheap_init(KHEAP_MAP_OFFSET, KHEAP_SIZE)) != 1) {
		printf("KHEAP: Initialization failed!\n");
		goto fatal_error;
	}

	printf("ACPI: Initialization ... ");
	void* rsdp_ptr = kboot_info->rsdp_version > 0 ? kboot_info->rsdp_data : NULL;
	if ((rc = acpi_init(rsdp_ptr)) != 0) {
		printf("ACPI: Init ERROR 0x%i\n", rc);
		goto fatal_error;
	}
	if ((rc = acpi_enable()) != 0) {
		printf("ACPI: Enable ERROR 0x%i\n", rc);
		goto fatal_error;
	} else {
		printf("Success!\n");
	}

	apic_init();
	smp_init();
	timer_init();

	init_interrupts_handler(); 
	printf("Reading PCI devices\n");
	load_pci_devices_list();	

	struct tm datetime;
	cmos_rtc_get_datetime_tm(&datetime);
	printf("%i:%i:%i   %i:%i:%i\n", datetime.tm_hour, 
									datetime.tm_min,
									datetime.tm_sec,
									datetime.tm_mday,
									datetime.tm_mon,
									datetime.tm_year + 1900);

	vfs_init();
	ext2_init();
	devfs_init();
	ahci_init();	
	init_nvme();

	vga_init_dev();
	
	init_ints_keyboard();

	for(int i = 0; i < get_drive_devices_count(); i ++) {
		drive_device_t* device = get_drive(i);
		add_partitions_from_device(device);
	}

	// Первоначальный процесс bootshell
	struct process* proc = create_new_process(NULL);
	// Добавить в список и назначить pid
    process_add_to_list(proc);
	struct thread* thr = create_kthread(proc, bootshell);
	
	init_scheduler();
	
	scheduler_add_thread(thr);
	scheduler_start();
	scheduler_yield();

	fatal_error:
		printf("Fatal Error!\nKernel terminated!");
		asm("hlt");
}
