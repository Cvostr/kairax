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
#include "dev/acpi/acpi.h"

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
			printf(P2V("thread 1 - %i \n"), i);
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

	acpi_init();

	init_pic();
	setup_idt();

	init_interrupts_handler(); 
	init_pmm();

	load_pci_devices_list();
	printf("PCI devices %i\n", get_pci_devices_count());	

	uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL | PAGE_USER_ACCESSIBLE;

	cmos_datetime_t datetime = cmos_rtc_get_datetime();
	printf("%i:%i:%i   %i:%i:%i\n", datetime.hour, datetime.minute, datetime.second, datetime.day, datetime.month, datetime.year);

	uint64_t npageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;
	page_table_t* new_pt = new_page_table();
	for(uintptr_t i = 0; i <= 1024 * 1024 * 32; i += 4096){
		map_page_mem(new_pt, i, i, npageFlags);
		map_page_mem(new_pt, P2V(i), i, npageFlags);
	}
	
	switch_pml4(new_pt);
	set_kernel_pml4(new_pt);

	char* p = (char *) (15ull*1024*1024*1024);
	map_page(new_pt, (uint64_t)(p), pageFlags);
	memcpy(p, "this text is in 15G offset\n", 28);
	print_string(p);

	ahci_init();	
	init_nvme();
	
	process_t* proc = create_new_process();
	process_t* proc1 = create_new_process();
	//process_brk(proc, (void *)0x1100000);
	//copy_to_vm(proc->pml4, 0x10000, (void *)(uintptr_t)threaded, 200);

	thread_t* thr = create_new_thread(proc, threaded);
	thread_t* thr2 = create_new_thread(proc1, threaded2);

	/*init_scheduler();

	add_thread(thr);
	add_thread(thr2);

	start_scheduler();*/
	

	

	while(1){

	}
}
