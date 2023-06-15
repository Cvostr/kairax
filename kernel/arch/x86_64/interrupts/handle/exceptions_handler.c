#include "exceptions_handler.h"
#include "handler.h"
#include "stdlib.h"
#include "stdio.h"
#include "memory/mem_layout.h"

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
		register_interrupt_handler(i, exception_handler);
	}
}

void exception_handler(interrupt_frame_t* frame){
    printf("Exception occured 0x%s (%s)\nKernel terminated. Please reboot your computer\n", 
    itoa(frame->int_no, 16), exception_message[frame->int_no]);
    printf("RAX = %s ", ulltoa(frame->rax, 16));
    printf("RBX = %s ", ulltoa(frame->rbx, 16));
    printf("RCX = %s\n", ulltoa(frame->rcx, 16));
    printf("RIP = %s ", ulltoa(frame->rip, 16));
    printf("RSP = %s ", ulltoa(frame->rsp, 16));
    printf("RBP = %s \n", ulltoa(frame->rbp, 16));
    printf("CS = %s ", ulltoa(frame->cs, 16));
    printf("SS = %s \n", ulltoa(frame->cs, 16));
    printf("STACK TRACE: ");
    uintptr_t* stack_ptr = (uintptr_t*)frame->rsp;
    for (int i = 0; i < 20; i ++) {
        uintptr_t value = *(stack_ptr++);
        if(value > KERNEL_TEXT_OFFSET)
            printf("%s, ", ulltoa(value, 16));
    }

	asm volatile("hlt");
}