#include "handler.h"
#include "kstdlib.h"

#include "exceptions_handler.h"

void (*interrupt_handlers[256])(interrupt_frame_t*);

void init_interrupts_handler(){
	register_exceptions_handlers();
}

void int_handler(interrupt_frame_t* frame){
	(interrupt_handlers[frame->int_no])(frame);
}

void register_interrupt_handler(int interrupt_num, void* handler_func){
	if(interrupt_num <= 255){
		interrupt_handlers[interrupt_num] = handler_func;
	}
}