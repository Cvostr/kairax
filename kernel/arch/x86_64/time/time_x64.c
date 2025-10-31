#include "dev/cmos/cmos.h"
#include "dev/hpet/hpet.h"

struct timeval __boot_time;

void arch_time_init()
{
    struct tm datetime;
    cmos_rtc_get_datetime_tm(&datetime);
    hpet_reset_counter();

    __boot_time.tv_sec = tm_to_epoch(&datetime);
    __boot_time.tv_usec = 0;
} 

void arch_sys_get_time_epoch(struct timeval *tv)
{
    uint64_t cntr = hpet_get_counter();
    uint64_t period = hpet_get_period();

    uint64_t ticks_per_sec = HPET_FEMTOSECONDS_PER_SEC / period;

    uint64_t secs = cntr / ticks_per_sec;
    uint64_t ticks_in_sec = cntr % ticks_per_sec;
    uint64_t microsecs = ticks_in_sec * period / HPET_FEMTOSECONDS_PER_MICROSEC;
    
    tv->tv_sec = __boot_time.tv_sec + secs;
    tv->tv_usec = microsecs;
}

void arch_sys_set_time_epoch(const struct timeval *tv)
{
    struct tm datetime;

    __boot_time.tv_sec = tv->tv_sec;
    __boot_time.tv_usec = 0;
    hpet_reset_counter();

    epoch_to_tm(&tv->tv_sec, &datetime);
    cmos_rtc_set_datetime_tm(&datetime);
}