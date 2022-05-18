#include "interrupts/handle/handler.h"
#include "thread_scheduler.h"
#include "stdio.h"
#include "interrupts/pic.h"
#include "list/list.h"

list_t* threads_list;
int curr_thread = 0;


void timer_int_handler(interrupt_frame_t* frame){
	
    printf("TIMER ");

	pic_eoi(0);
}

void init_scheduler(){
    threads_list = create_list();

	register_interrupt_handler(0x20, timer_int_handler);
    pic_unmask(0x20);
}