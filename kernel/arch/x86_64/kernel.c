#include "dev/b8-console/b8-console.h"
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

void kmain(uint32_t multiboot_magic, void* multiboot_struct_ptr){
	b8_set_addr(P2K(0xB8000));
	b8_console_clear();
	printf("Kairax Kernel v0.1\n");

	parse_mb2_tags(multiboot_struct_ptr);
	printf("LOADER : %s\n", get_kernel_boot_info()->bootloader_string);
	printf("CMDLINE : %s\n", get_kernel_boot_info()->command_line);
	//printf("BASE : %i\n", get_kernel_boot_info()->load_base_addr);

	init_pmm();

	pmm_params_t pmm_params;
	memset(&pmm_params, 0, sizeof(pmm_params_t));
	pmm_params.kernel_base = get_kernel_boot_info()->load_base_addr;
	// Проходимся по регионам grub
	for (int i = 0; i < get_kernel_boot_info()->mmap_len; i ++) {
		uintptr_t start, end;
		uint32_t type;
		multiboot_get_memory_area(i, &start, &end, &type);
		
		if (end > pmm_params.physical_mem_max_addr)
			pmm_params.physical_mem_max_addr = end;
		
		//Считаем физическую память
		pmm_params.physical_mem_total += (end - start);

		if (type != 1)
			pmm_set_mem_region(start, end - start);

		//printf("REGION : %i %i %i\n", start, end, type);
	}

	pmm_take_base_regions();
	pmm_set_params(&pmm_params);

	init_pic();
	setup_idt();
	
	printf("VMM: Creating kernel memory map\n");
	page_table_t* new_pt = create_kernel_vm_map();
	printf("VMM: Setting kernel memory map\n");

	//ASUS
	switch_pml4(K2P(new_pt));

	b8_set_addr(P2V(0xB8000));
	//VBOX

	int rc = 0;
	printf("KHEAP: Initialization...\n");
	if ((rc = kheap_init(KHEAP_MAP_OFFSET, KHEAP_SIZE)) != 1) {
		printf("KHEAP: Initialization failed!\n");
		goto fatal_error;
	}

	printf("ACPI: Initialization ...\n");
	acpi_init();
	printf("Enabling ACPI ...  ");
	if((rc = acpi_enable()) != 0) {
		printf("ACPI: ERROR 0x%i\n", rc);
		goto fatal_error;
	} else {
		printf("Success!\n");
	}

	apic_init();
	smp_init();

	init_interrupts_handler(); 
	init_ints_keyboard();

	printf("Reading PCI devices\n");
	load_pci_devices_list();	

	cmos_datetime_t datetime = cmos_rtc_get_datetime();
	printf("%i:%i:%i   %i:%i:%i\n", datetime.hour, datetime.minute, datetime.second, datetime.day, datetime.month, datetime.year);

	vfs_init();
	ext2_init();
	devfs_init();
	ahci_init();	
	init_nvme();

	for(int i = 0; i < get_drive_devices_count(); i ++) {
		drive_device_t* device = get_drive(i);
		add_partitions_from_device(device);
	}

	// Первоначальный процесс bootshell
	struct process* proc = create_new_process(NULL);
	struct thread* thr = create_kthread(proc, bootshell);
	
	init_scheduler();
	
	scheduler_add_thread(thr);
	scheduler_start();

	while(1){

	}

	fatal_error:
		printf("Fatal Error!\nKernel terminated!");
		asm("hlt");
}
