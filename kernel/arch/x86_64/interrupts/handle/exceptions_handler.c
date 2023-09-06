#include "exceptions_handler.h"
#include "handler.h"
#include "kstdlib.h"
#include "stdio.h"
#include "memory/mem_layout.h"
#include "proc/process.h"
#include "cpu/cpu_local_x64.h"

void exception_handler(interrupt_frame_t* frame); //; прототип

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

void register_exceptions_handlers(){
	for(int i = 0; i < 32; i ++){
		register_interrupt_handler(i, exception_handler, NULL);
	}
}

void exception_handler(interrupt_frame_t* frame)
{
    uint64_t cr2;
    asm volatile ("mov %%cr2, %%rax\n mov %%rax, %0" : "=m" (cr2));

    if (frame->cs == 0x23) {
        // Исключение произошло в пользовательском процессе
        
        if (frame->int_no == 0xE) {
            printf("PF in user");
            int rc = process_handle_page_fault(cpu_get_current_thread()->process, cr2);

            if (rc == 1) {
                // Все нормально, можно выходить
                return;
            }
        }
    }

    printf("Exception occured 0x%s (%s)\nKernel terminated. Please reboot your computer\n", 
    itoa(frame->int_no, 16), exception_message[frame->int_no]);
    printf("RAX = %s ", ulltoa(frame->rax, 16));
    printf("RBX = %s ", ulltoa(frame->rbx, 16));
    printf("RCX = %s\n", ulltoa(frame->rcx, 16));
    printf("RIP = %s ", ulltoa(frame->rip, 16));
    printf("RSP = %s ", ulltoa(frame->rsp, 16));
    printf("RBP = %s \n", ulltoa(frame->rbp, 16));
    printf("CS = %s ", ulltoa(frame->cs, 16));
    printf("SS = %s ", ulltoa(frame->cs, 16));
    printf("CR2 = %s \n", ulltoa(cr2, 16));
    printf("STACK TRACE: ");
    uintptr_t* stack_ptr = (uintptr_t*)frame->rsp;
    for (int i = 0; i < 20; i ++) {
        uintptr_t value = *(stack_ptr++);
        if(value > KERNEL_TEXT_OFFSET)
            printf("%s, ", ulltoa(value, 16));
    }

	asm volatile("hlt");
}