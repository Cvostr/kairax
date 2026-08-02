#include "handler.h"
#include "kstdlib.h"
#include "interrupts/pic.h"
#include "exceptions_handler.h"
#include "../apic.h"
#include "../ioapic.h"
#include "kairax/signal.h"
#include "kairax/string.h"
#include "dev/interrupts.h"
#include "cpu/cpu_local_x64.h"
#include "mem/kheap.h"

typedef void (*interrupt_handler) (interrupt_frame_t*, void*);

struct irq_handler
{
	interrupt_handler handler;
	void* data;
	struct irq_handler *next;
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

	// Получаем первый обработчик в цепочке
	struct irq_handler *ihandler = &irq_handlers[frame->int_no];

	// Вызываем все обработчики в этой цепочке
	while (ihandler != NULL) 
	{
		interrupt_handler handler = ihandler->handler;
		void* data = ihandler->data;
		
		if (handler)
			handler(frame, data);

		ihandler = ihandler->next;
	}

	if (frame->cs == 0x23) {
		process_handle_signals(CALLER_INTERRUPT, frame);
	}

	if (frame->int_no >= 0x20) {
		lapic_eoi();
	}

	struct tasklet_list* scheduled_tasklets = this_core->scheduled_tasklets;
	if (scheduled_tasklets->head != NULL && cpu_get_current_thread() != NULL)
	{
		scheduled_tasklets->changed = 1;
		scheduler_yield(TRUE);
	}
}

int register_interrupt_handler(int interrupt_num, void* handler_func, void* data)
{
	if (interrupt_num <= 255) 
	{
		if (irq_handlers[interrupt_num].handler == NULL)
		{
			irq_handlers[interrupt_num].handler = handler_func;
			irq_handlers[interrupt_num].data = data; 
		}
		else
		{
			// Обработчик уже был назначен на этот вектор прерывания
			// Создаем объект нового обработчика
			struct irq_handler *new_handler = kmalloc(sizeof(struct irq_handler));
			if (new_handler == NULL)
				return -ENOMEM;
				
			new_handler->handler = handler_func;
			new_handler->data = data; 
			new_handler->next = NULL;

			// Ищем последний объект в цепочке
			struct irq_handler *parent = &irq_handlers[interrupt_num];
			while (parent->next != NULL)
				parent = parent->next;

			// Добавляем созданный объект в цепочку	
			parent->next = new_handler;
		}

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