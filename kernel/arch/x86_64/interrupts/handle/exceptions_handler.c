#include "exceptions_handler.h"
#include "handler.h"
#include "kstdlib.h"
#include "stdio.h"
#include "memory/mem_layout.h"
#include "proc/process.h"
#include "cpu/cpu_local_x64.h"
#include "memory/mem_layout.h"
#include "mem/pmm.h"
#include "memory/kernel_vmm.h"
#include "../apic.h"
#include "cpu/cpu.h"

char const *exception_message[33] =
{
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor (not available)",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS", // Task State Segment
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt (reserved)",
    "Coprocessor (FPE) Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD (FPE)",
    "Virtualization exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security exception",
    "Reserved",
    "Reserved"
};

//#define PAGE_PROTECTION_VIOLATION

void exception_handler(interrupt_frame_t* frame)
{
    uint64_t cr2, cr3;
    asm volatile ("mov %%cr2, %%rax\n mov %%rax, %0" : "=m" (cr2));
    asm volatile ("mov %%cr3, %%rax\n mov %%rax, %0" : "=m" (cr3));

    if (frame->int_no == EXCEPTION_PAGE_FAULT && (frame->error_code & 0b0001) == 0) {

        // Page Fault
        if (cr2 >= 0 && cr2 <= USERSPACE_MAX_ADDR) {
            
            struct thread* thr = cpu_get_current_thread();
            if (thr) {
                int rc = process_handle_page_fault(thr->process, cr2);

                if (rc == 1) {
                    // Все нормально, можно выходить
                    return;
                }
            }
        } else if (cr2 >= PHYSICAL_MEM_MAP_OFFSET && cr2 <= PHYSICAL_MEM_MAP_END) {
            uint64_t addr_aligned = align_down(cr2, PAGE_SIZE);
            uint64_t phys_addr = V2P(addr_aligned);
            uint64_t pageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;
            map_page_mem( get_kernel_pml4(), (virtual_addr_t) addr_aligned, phys_addr, pageFlags);
            return;
        }
    }

    printk("Exception occured 0x%s (%s)\n", 
    itoa(frame->int_no, 16), exception_message[frame->int_no]);
    printk("ERR = %s\n", ulltoa(frame->error_code, 16));
    printk("RAX = %s ", ulltoa(frame->rax, 16));
    printk("RBX = %s ", ulltoa(frame->rbx, 16));
    printk("RCX = %s\n", ulltoa(frame->rcx, 16));
    printk("RIP = %s ", ulltoa(frame->rip, 16));
    printk("RSP = %s ", ulltoa(frame->rsp, 16));
    printk("RBP = %s\n", ulltoa(frame->rbp, 16));
    printk("RDI = %s ", ulltoa(frame->rdi, 16));
    printk("RSI = %s ", ulltoa(frame->rsi, 16));
    printk("RDX = %s\n", ulltoa(frame->rdx, 16));
    printk("CS = %s ", ulltoa(frame->cs, 16));
    printk("SS = %s ", ulltoa(frame->ss, 16));
    printk("CR2 = %s ", ulltoa(cr2, 16));
    printk("CR3 = %s\n", ulltoa(cr3, 16));

    uint8_t* ip = (uint8_t*) frame->rip;
    int show_instr = 1;
    if (cpu_get_current_vm_table() != NULL) {
        show_instr = vm_is_mapped(cpu_get_current_vm_table(), ip);
    }
    if (show_instr == 1) {
        printk("INSTR: ");
        for (int i = 0; i < 8; i ++) {
            printk("%s ", ulltoa(*(ip++), 16));
        }
    }
    
    uintptr_t* stack_ptr = (uintptr_t*)frame->rsp;
    int show_stack = 1;
    if (cpu_get_current_vm_table() != NULL) {
        show_stack = vm_is_mapped(cpu_get_current_vm_table(), stack_ptr);
    }
    if (show_stack) {
        printk("\nSTACK TRACE: \n");
        for (int i = 0; i < 25; i ++) {
            uintptr_t value = *(stack_ptr + i);
            printk("%s ", ulltoa(value, 16));
        }
    }
    
    printk("\n");

    if (frame->cs == 0x23) {
        // Исключение произошло в пользовательском процессе
        // Завершаем процесс
        int rc = -1;
        switch (frame->int_no) {
            case EXCEPTION_PAGE_FAULT:
                rc = 128 + SIGSEGV;
                break;
            case EXCEPTION_INVALID_OPCODE:
                rc = 128 + SIGILL;
                break;
        }
        printk("Process terminated!\n");
        sys_exit_process(rc);

    } else {
        printk("Kernel terminated. Please reboot your computer\n");

        // Остановить остальные ядра
        lapic_send_ipi(0, IPI_DST_OTHERS, IPI_TYPE_FIXED, INTERRUPT_VEC_HLT);
    }

	asm volatile("hlt");
}