#include "base/x86-console/x86-console.h"
#include "stdio.h"
#include "stdlib.h"
#include "interrupts/idt.h"
#include "interrupts/pic.h"
#include "interrupts/handle/handler.h"
#include "dev/keyboard/int_keyboard.h"
#include "dev/pci/pci.h"

#include "dev/ahci/ahci.h"

#include "memory/hh_offset.h"
#include "memory/pmm.h"
#include "memory/paging.h"

#include "boot/multiboot.h"
#include "string.h"

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

	print_string("Interrupts initialized\n");
	
	load_pci_devices_list();
	printf("PCI devices %i\n", get_pci_devices_count());	

	uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL | PAGE_USER_ACCESSIBLE;
	init_pmm();

	page_table_t* pt = (void*)0x608000;
	map_page(get_kernel_pml4(), (uint64_t)(15ull*1024*1024*1024), pageFlags);
	map_page(get_kernel_pml4(), (uint64_t)0xDEADBEAF, pageFlags);
	//switch_pml4(V2P(get_kernel_pml4()));

  	//uint64_t* p = (uint64_t *) (0xDEADBEAF);
	uint64_t* p = (uint64_t *) (15ull*1024*1024*1024);
	memcpy(p, "xui pizda\n", 11);
	print_string(p);

	printf("%i\n", get_physical_address(get_kernel_pml4(), 15ull*1024*1024*1024));
	printf("%i\n", get_physical_address(get_kernel_pml4(), 0xDEADBEAF));

	printf("IS MAPPED %i\n", is_mapped(get_kernel_pml4(), 0xDEADBEAF));

	print_string("Paging\n");

	ahci_init();	
	
	while(1){
	
	}
}
