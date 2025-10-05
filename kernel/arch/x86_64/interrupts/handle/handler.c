#include "handler.h"
#include "kstdlib.h"
#include "interrupts/pic.h"
#include "exceptions_handler.h"
#include "../apic.h"
#include "../ioapic.h"
#include "kairax/signal.h"
#include "kairax/string.h"

typedef void (*interrupt_handler) (interrupt_frame_t*, void*);

struct irq_handler
{
	interrupt_handler handler;
	void* data;
};

struct irq_handler irq_handlers[256];

void init_interrupts_handler()
{
	memset(&irq_handlers, 0, sizeof(irq_handlers));
}

void int_handler(interrupt_frame_t* frame) 
{
	if (frame->int_no < 0x20) {
		exception_handler(frame);
	
		if (frame->cs == 0x23) {
			process_handle_signals(CALLER_INTERRUPT, frame);
		}

		return;
	}

	interrupt_handler handler = irq_handlers[frame->int_no].handler;

	if (handler)
		handler(frame, irq_handlers[frame->int_no].data);

	if (frame->cs == 0x23) {
		process_handle_signals(CALLER_INTERRUPT, frame);
	}

	if (frame->int_no >= 0x20) {
		lapic_eoi();
	}
}

int register_interrupt_handler(int interrupt_num, void* handler_func, void* data)
{
	if (interrupt_num <= 255) 
	{
		irq_handlers[interrupt_num].handler = handler_func;
		irq_handlers[interrupt_num].data = data; 

		return 0;
	}

	return -1;
}

int register_irq_handler(int irq, void* handler, void* data)
{
	int rc = register_interrupt_handler(0x20 + irq, handler, data);
	if (rc != 0)
	{
		return rc;
	}

	ioapic_redirect_interrupt(0, 0x20 + irq, irq);

	return 0;
}