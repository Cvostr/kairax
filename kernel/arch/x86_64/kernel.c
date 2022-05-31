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
#include "mem/kheap.h"

#include "boot/multiboot.h"
#include "string.h"

#include "proc/thread_scheduler.h"
#include "dev/cmos/cmos.h"
#include "dev/acpi/acpi.h"
#include "drivers/storage/devices/storage_devices.h"
#include "drivers/storage/partitions/storage_partitions.h"

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
	clear_console();
	print_string("Kairax Kernel v0.1\n");

	parse_mb2_tags(multiboot_struct_ptr);
	printf("LOADER : %s\n", get_kernel_boot_info()->bootloader_string);
	printf("CMDLINE : %s\n", get_kernel_boot_info()->command_line);

	acpi_init();
	for(uint32_t i = 0; i < acpi_get_cpus_apic_count(); i ++){
		printf("APIC on CPU %i Id : %i \n", acpi_get_cpus_apic()[i]->acpi_cpu_id, acpi_get_cpus_apic()[i]->apic_id);
	}

	init_pic();
	setup_idt();

	init_interrupts_handler(); 
	init_ints_keyboard();
	init_pmm();

	load_pci_devices_list();
	printf("PCI devices %i\n", get_pci_devices_count());	

	cmos_datetime_t datetime = cmos_rtc_get_datetime();
	printf("%i:%i:%i   %i:%i:%i\n", datetime.hour, datetime.minute, datetime.second, datetime.day, datetime.month, datetime.year);

	uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;
	page_table_t* new_pt = new_page_table();
	for(uintptr_t i = 0; i <= KERNEL_MEMORY_SIZE; i += 4096){
		map_page_mem(new_pt, i, i, pageFlags);
		map_page_mem(new_pt, P2V(i), i, pageFlags);
	}
	
	switch_pml4(new_pt);
	set_kernel_pml4(new_pt);

	virtual_addr_t addr = P2V(KERNEL_MEMORY_SIZE);
	kheap_init(addr, KHEAP_PAGES_SIZE * 4096);

	ahci_init();	
	init_nvme();

	for(int i = 0; i < get_drive_devices_count(); i ++){
		drive_device_header_t* device = get_drive_devices()[i];
		printf("Drive Name %s, Model %s, Size : %i MiB\n", device->name, device->model, device->bytes / (1024UL * 1024));
		add_partitions_from_device(device);
	}

	for(int i = 0; i < get_partitions_count(); i ++){
		drive_partition_header_t* partition = get_partition(i);
		printf("Partition Name %s, Index : %i, Start : %i, Size : %i\n", partition->name, partition->index,
		 																	partition->start_sector, partition->sectors);
	}

	process_t* proc = create_new_process();
	process_t* proc1 = create_new_process();
	//process_brk(proc, (void *)0x1100000);
	//copy_to_vm(proc->pml4, 0x10000, (void *)(uintptr_t)threaded, 200);

	thread_t* thr = create_new_thread(proc, threaded);
	thread_t* thr2 = create_new_thread(proc1, threaded2);

	init_scheduler();

	add_thread(thr);
	add_thread(thr2);

	//start_scheduler();


	

	while(1){

	}
}
