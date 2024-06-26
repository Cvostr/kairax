#include "handler.h"
#include "kstdlib.h"
#include "interrupts/pic.h"
#include "exceptions_handler.h"
#include "../apic.h"
#include "../ioapic.h"
#include "kairax/signal.h"

void (*interrupt_handlers[256])(interrupt_frame_t*, void*);
void* interrupt_datas[256];

void init_interrupts_handler(){
	
}

void int_handler(interrupt_frame_t* frame) 
{
	if (frame->int_no < 0x20) {
		exception_handler(frame);
	
		if (frame->cs == 0x23) {
			process_handle_signals();
		}

		return;
	}

	void (*handler)() = interrupt_handlers[frame->int_no];

	if (handler)
		handler(frame, interrupt_datas[frame->int_no]);

	if (frame->cs == 0x23) {
		process_handle_signals();
	}

	if (frame->int_no >= 0x20) {
		lapic_eoi();
	}
}

void register_interrupt_handler(int interrupt_num, void* handler_func, void* data)
{
	if (interrupt_num <= 255) {
		interrupt_handlers[interrupt_num] = handler_func;
		interrupt_datas[interrupt_num] = data;
	}
}

int register_irq_handler(int irq, void* handler, void* data)
{
	register_interrupt_handler(0x20 + irq, handler, data);

	ioapic_redirect_interrupt(0, 0x20 + irq, irq);
}