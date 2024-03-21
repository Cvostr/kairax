#include "timer.h"
#include "pit.h"
#include "proc/x64_context.h"
#include "proc/thread_scheduler.h"
#include "interrupts/pic.h"
#include "proc/timer.h"
#include "cpu/cpu_local_x64.h"

extern int scheduler_handler(thread_frame_t* frame);

void timer_int_handler(thread_frame_t* context) 
{
    int lc = cpu_get_lapic_id();

    if (lc == 0) {
        timer_handle();
    } else {
        lapic_eoi();
        return;
    }

    lapic_eoi();

    scheduler_handler(context);
}

void arch_timer_init()
{
    //pit_set_frequency(TIMER_FREQUENCY);

    //ioapic_redirect_interrupt(0, 0x20, 0);
}