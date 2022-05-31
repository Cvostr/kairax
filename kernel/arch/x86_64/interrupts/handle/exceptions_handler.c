#include "exceptions_handler.h"
#include "handler.h"
#include "stdlib.h"
#include "stdio.h"

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
	asm volatile("hlt");
}