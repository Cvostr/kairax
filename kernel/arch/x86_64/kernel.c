#include "dev/b8-console/b8-console.h"
#include "stdio.h"
#include "stdlib.h"
#include "interrupts/idt.h"
#include "interrupts/pic.h"
#include "interrupts/handle/handler.h"
#include "dev/keyboard/int_keyboard.h"
#include "bus/pci/pci.h"

#include "drivers/storage/ahci/ahci.h"
#include "drivers/storage/nvme/nvme.h"

#include "memory/hh_offset.h"
#include "mem/pmm.h"
#include "memory/paging.h"
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

#include "misc/bootshell/bootshell.h"

#define KERNEL_MEMORY_SIZE (1024ULL * 1024 * 32)
#define KHEAP_PAGES_SIZE 4096		//16MB

void threaded(){
	//asm volatile("hlt");
	while(1){
		for(int i = 0; i < 100000000; i ++){
			asm volatile("nop");
		}
		for(int i = 0; i < 3; i ++){
			for(int i = 0; i < 10000000; i ++){
				asm volatile("nop");
			}
			printf("thread 1 - %i \n", i);
		}
	}
}

void threaded2(){
	while(1){
		for(int i = 0; i < 10000000; i ++){
			asm volatile("nop");
		}

		printf("thread 2 \n");
	}
}

void kmain(uint multiboot_magic, void* multiboot_struct_ptr){
	b8_console_clear();
	printf("Kairax Kernel v0.1\n");

	parse_mb2_tags(multiboot_struct_ptr);
	printf("LOADER : %s\n", get_kernel_boot_info()->bootloader_string);
	printf("CMDLINE : %s\n", get_kernel_boot_info()->command_line);

	init_pmm();
	uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;
	page_table_t* new_pt = new_page_table();
	for(uintptr_t i = 0; i <= KERNEL_MEMORY_SIZE; i += 4096){
		map_page_mem(new_pt, i, i, pageFlags);
		map_page_mem(new_pt, P2V(i), i, pageFlags);
	}
	switch_pml4(new_pt);
	set_kernel_pml4(new_pt);

	acpi_init();
	acpi_enable();

	init_pic();
	setup_idt();

	init_interrupts_handler(); 
	init_ints_keyboard();

	load_pci_devices_list();	

	cmos_datetime_t datetime = cmos_rtc_get_datetime();
	printf("%i:%i:%i   %i:%i:%i\n", datetime.hour, datetime.minute, datetime.second, datetime.day, datetime.month, datetime.year);

	virtual_addr_t addr = P2V(KERNEL_MEMORY_SIZE);
	kheap_init(addr, KHEAP_PAGES_SIZE * 4096);

	vfs_init();
	ext2_init();
	ahci_init();	
	init_nvme();

	for(int i = 0; i < get_drive_devices_count(); i ++){
		drive_device_t* device = get_drive(i);
		add_partitions_from_device(device);
	}

	process_t* proc = create_new_process();
	process_t* proc1 = create_new_process();
	//process_brk(proc, (void *)0x1100000);
	//copy_to_vm(proc->pml4, 0x10000, (void *)(uintptr_t)threaded, 200);

	thread_t* thr = create_new_thread(proc, bootshell);
	//thread_t* thr2 = create_new_thread(proc1, threaded2);

	init_scheduler();

	add_thread(thr);
	//add_thread(thr2);

	start_scheduler();


	

	while(1){

	}
}
