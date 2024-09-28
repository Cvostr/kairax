#include "drivers/video/video.h"
#include "stdio.h"
#include "kstdlib.h"
#include "interrupts/idt.h"
#include "interrupts/pic.h"
#include "interrupts/handle/handler.h"
#include "dev/keyboard/int_keyboard.h"
#include "dev/bus/pci/pci.h"

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
#include "dev/device_man.h"
#include "drivers/storage/partitions/storage_partitions.h"

#include "fs/vfs/vfs.h"
#include "fs/ext2/ext2.h"
#include "fs/devfs/devfs.h"

#include "misc/bootshell/bootshell.h"
#include "cpu/gdt.h"
#include "cpu/msr.h"
#include "cpu/cpu.h"
#include "interrupts/apic.h"
#include "cpu/cpuid.h"
#include "proc/timer.h"
#include "proc/idle.h"
#include "drivers/tty/tty.h"
#include "drivers/char/random.h"
#include "misc/kterm/kterm.h"
#include "misc/kterm/vgaterm.h"
#include "kairax/intctl.h"

extern struct vgaconsole* current_console;

extern void x64_full_halt();

extern int kairax_version_major;
extern int kairax_version_minor;
extern const char* kairax_build_date;
extern const char* kairax_build_time;

void halt();

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
		
		if (type == 1) {
			//Считаем физическую память
			pmm_params.physical_mem_total += length;
		}

		// Запоминаем регион
		pmm_add_region(start, length, type == 1 ? PMM_REGION_USABLE : 0);

		if (type != 1)
			pmm_set_mem_region(start, length);
	}

	pmm_take_base_regions();
	pmm_set_params(&pmm_params);

	init_pic();
	setup_idt();
	init_interrupts_handler(); 
	// Установить IDT
    load_idt();
	// включение прерываний
	enable_interrupts();
	
	page_table_t* new_pt = create_kernel_vm_map();
	vmm_use_kernel_vm();

	// ----------- KHEAP ----------
	kheap_init(KHEAP_MAP_OFFSET, KHEAP_SIZE);
	// ----------------------------

	vga_init(
		P2V(kboot_info->fb_info.fb_addr),
		kboot_info->fb_info.fb_pitch,
		kboot_info->fb_info.fb_width,
		kboot_info->fb_info.fb_height,
		kboot_info->fb_info.fb_bpp);

	current_console = console_init();

	printk("Kairax Kernel v%i.%i (%s %s)\n", kairax_version_major, kairax_version_minor, kairax_build_date, kairax_build_time);

	/*struct pmm_region* reg = pmm_get_regions();
	while (reg->length != 0) {
		printf("REGION : %i %i %i\n", reg->base, reg->length, reg->flags);
		reg++;
	} */

	//printf("LOADER : %s\n", kboot_info->bootloader_string);
	//printf("CMDLINE : %s\n", P2V(kboot_info->command_line));

	int rc = 0;

	if (rc = fpu_init() != 0) {
		printk("FPU init error: %i", rc);
		goto fatal_error;
	}

	printk("ACPI: Initialization ... ");
	void* rsdp_ptr = kboot_info->rsdp_version > 0 ? kboot_info->rsdp_data : NULL;
	if ((rc = acpi_init(rsdp_ptr)) != 0) {
		printk("ACPI: Init ERROR 0x%i\n", rc);
		goto fatal_error;
	}
	if ((rc = acpi_enable()) != 0) {
		printk("ACPI: Enable ERROR 0x%i\n", rc);
		goto fatal_error;
	} else {
		printk("Success!\n");
	}

	printk("APIC: Initialization\n");
	if (apic_init() != 0) {
		goto fatal_error;
	}

	timer_init();

	printk("SMP: Initialization\n");
	init_idle_process();
	if (smp_init() != 0) {
		goto fatal_error;
	}

	printk("Reading PCI devices\n");
	load_pci_devices_list();	

	struct tm datetime;
	cmos_rtc_get_datetime_tm(&datetime);
	printk("%i:%i:%i   %i:%i:%i\n", datetime.tm_hour, 
									datetime.tm_min,
									datetime.tm_sec,
									datetime.tm_mday,
									datetime.tm_mon + 1,
									datetime.tm_year + 1900);

	vfs_init();
	ext2_init();
	devfs_init();
	ahci_init();	
	init_nvme();
	tty_init();

	vga_init_dev();
	random_init();
	zero_init();
	init_ints_keyboard();

	usb_init();
	net_init();
	
	register_interrupt_handler(INTERRUPT_VEC_RES, cpu_reschedule_ipi, 0);
	register_interrupt_handler(INTERRUPT_VEC_HLT, x64_full_halt, 0);
	register_interrupt_handler(INTERRUPT_VEC_TLB, cpu_tlb_shootdown_ipi, 0);

	struct device* dev = NULL;
    int i = 0;
    while ((dev = get_device(i ++)) != NULL) {

        if(dev != NULL) {
            
            if (dev->dev_type == DEVICE_TYPE_DRIVE) {
                add_partitions_from_device(dev);
            }
        }
    }

	kterm_process_start();

	init_scheduler();
	scheduler_start();
	scheduler_yield(FALSE);

	fatal_error:
		printk("Fatal Error!\nKernel terminated!");
		x64_full_halt();
}