#include "base/x86-console/x86-console.h"
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
#include "memory/pmm.h"
#include "memory/paging.h"

#include "boot/multiboot.h"
#include "string.h"

#include "proc/thread_scheduler.h"
#include "dev/cmos/cmos.h"

void threaded(){
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
	clear_console();
	print_string("Kairax Kernel v0.1\n");

	parse_mb2_tags(multiboot_struct_ptr);
	printf("LOADER : %s\n", get_kernel_boot_info()->bootloader_string);
	printf("CMDLINE : %s\n", get_kernel_boot_info()->command_line);

	init_pic();
	setup_idt();

	init_interrupts_handler();
	init_ints_keyboard();
	
	init_pmm();

	load_pci_devices_list();
	printf("PCI devices %i\n", get_pci_devices_count());	

	uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL | PAGE_USER_ACCESSIBLE;

	cmos_datetime_t datetime = cmos_rtc_get_datetime();
	printf("%i:%i:%i   %i:%i:%i\n", datetime.hour, datetime.minute, datetime.second, datetime.day, datetime.month, datetime.year);

	page_table_t* new_pt = new_page_table();
	memcpy(new_pt, get_kernel_pml4(), 4096);
	//for(uint32_t i = 0; i < 1024 * 1024 * 20; i += 4096){
	//	map_page_mem(new_pt, i, i, PAGE_PRESENT | PAGE_WRITABLE);
	//}
	
	map_page(new_pt, (uint64_t)(15ull*1024*1024*1024), pageFlags);
	switch_pml4(new_pt);

	char* p = (char *) (15ull*1024*1024*1024);
	memcpy(p, "this text is in 15G offset\n", 28);
	print_string(p);

	ahci_init();	
	init_nvme();
	
	process_t* proc = create_new_process();
	process_brk(proc, (void *)0x10100);

	thread_t* thr = create_new_thread(proc, threaded);
	thread_t* thr2 = create_new_thread(NULL, threaded2);
	

	init_scheduler();

	add_thread(thr);
	add_thread(thr2);

	start_scheduler();

	while(1){

	}
}
