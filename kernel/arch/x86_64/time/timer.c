#include "timer.h"
#include "pit.h"
#include "proc/x64_context.h"
#include "proc/thread_scheduler.h"
#include "interrupts/pic.h"
#include "proc/timer.h"

extern int scheduler_handler(thread_frame_t* frame);

void timer_int_handler(thread_frame_t* context) 
{
    pic_eoi(0);
    timer_handle();
    scheduler_handler(context);
}

void arch_timer_init()
{
    pit_set_frequency(TIMER_FREQUENCY);
    pic_unmask(0x20);
}