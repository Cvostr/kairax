#include "timer.h"
#include "pit.h"
#include "proc/x64_context.h"
#include "proc/thread_scheduler.h"

extern void scheduler_handler(thread_frame_t* frame);

void timer_int_handler(thread_frame_t* context) 
{
    pic_eoi(0);
    
    scheduler_handler(context);
}

void timer_init()
{
    pit_set_frequency(1000);

    //pic_unmask(0x20);
}