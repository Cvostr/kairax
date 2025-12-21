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
#include "cpu/cpu_x64.h"
#include "sync/spinlock.h"

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

spinlock_t dump_lock = 0;

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

    acquire_spinlock(&dump_lock);

    printk("Exception occured 0x%x (%s) on CPU %i\n", 
        frame->int_no, exception_message[frame->int_no], cpu_get_id());
    printk("ERR = %x\n", frame->error_code);
    printk("RAX = %x RBX = %x RCX = %x\n", frame->rax, frame->rbx, frame->rcx);
    printk("RIP = %x RSP = %x RBP = %x\n", frame->rip, frame->rsp, frame->rbp);
    printk("RDI = %x RSI = %x RDX = %x\n", frame->rdi, frame->rsi, frame->rdx);
    printk("CS = %x SS = %x ", frame->cs, frame->ss);
    printk("CR2 = %x CR3 = %x\n", cr2, cr3);

    uint8_t* ip = (uint8_t*) frame->rip;
    int show_instr = 1;
    if (cpu_get_current_vm_table() != NULL) {
        show_instr = vm_is_mapped(cpu_get_current_vm_table(), ip);
    }
    if (show_instr == 1) {
        printk("INSTR: ");
        for (int i = 0; i < 8; i ++) {
            printk("%x ", *(ip++));
        }
    }
    
    uintptr_t* stack_ptr = (uintptr_t*)frame->rsp;
    int show_stack = 1;
    if (cpu_get_current_vm_table() != NULL) {
        show_stack = vm_is_mapped(cpu_get_current_vm_table(), stack_ptr);
    }
    if (show_stack) {
        printk("\nSTACK TRACE: \n");
        /*
        for (int i = -10; i < 0; i ++) {
            uintptr_t value = *(stack_ptr + i);
            printk("%s ", ulltoa(value, 16));
        }
        printk("\n---------------\n");
        */
        for (int i = 0; i < 35; i ++) {
            uintptr_t value = *(stack_ptr + i);
            printk("%x ", value);
        }
    }
    
    printk("\n");

    if (frame->cs == 0x8) {
        printk("Kernel terminated. Please reboot your computer\n");
    }

    release_spinlock(&dump_lock);

    if (frame->cs == 0x23) {
        // Исключение произошло в пользовательском процессе
        // Завершаем процесс
        int signal = 0;
        switch (frame->int_no) {
            case EXCEPTION_DIVISION_BY_ZERO:
            case EXCEPTION_SIMD:
            case EXCEPTION_FPU_FAULT:
                signal = SIGFPE;
                break;
            case EXCEPTION_PAGE_FAULT:
                signal = SIGSEGV;
                break;
            case EXCEPTION_INVALID_OPCODE:
                signal = SIGILL;
                break;
            default:
                signal = SIGABRT;
                break;
        }

        thread_send_signal(cpu_get_current_thread(), signal);
        return;

    } else {

        // Остановить остальные ядра
        lapic_send_ipi(0, IPI_DST_OTHERS, IPI_TYPE_FIXED, INTERRUPT_VEC_HLT);
    }

	asm volatile("hlt");
}