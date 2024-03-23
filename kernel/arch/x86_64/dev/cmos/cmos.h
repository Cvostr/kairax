#ifndef _CMOS_H
#define _CMOS_H

#include "types.h"
#include "kairax/time.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define CMOS_REG_RTC_SEC 	    0x00
#define CMOS_REG_RTC_MIN 	    0x02
#define CMOS_REG_RTC_HOUR 	    0x04

#define CMOS_REG_RTC_WEEKDAY 	0x06
#define CMOS_REG_RTC_MONTHDAY   0x07
#define CMOS_REG_RTC_MONTH	    0x08
#define CMOS_REG_RTC_YEAR	    0x09
#define CMOS_REG_RTC_CENTURY    0x32

#define CMOS_STATUS_A           0x0A

unsigned long read_tsc();

void cmos_rtc_get_datetime_tm(struct tm* time);

void cmos_rtc_set_datetime_tm(struct tm* time);

void arch_sys_get_time_epoch(struct timeval *tv);
void arch_sys_set_time_epoch(const struct timeval *tv);

#endif
