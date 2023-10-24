#include "cmos.h"
#include "io.h"

static unsigned short from_bcd(unsigned short value)
{
    return (value / 16) * 10 + (value & 0xf);
}

unsigned short cmos_read(unsigned cmos_reg)
{
    outb(CMOS_ADDRESS, cmos_reg);
    return inb(CMOS_DATA);
}

int cmos_is_update()
{
    return cmos_read(CMOS_STATUS_A) & 0x80;
}

unsigned long read_tsc() {
	unsigned lo, hi;
	asm volatile ( "rdtsc" : "=a"(lo), "=d"(hi) );
	return ((unsigned long)hi << 32UL) | (unsigned long)lo;
}

void cmos_rtc_get_datetime_tm(struct tm* time)
{
    int century = from_bcd(cmos_read(CMOS_REG_RTC_CENTURY));
    int year_offset = century * 100 - 1900;

    time->tm_sec = from_bcd(cmos_read(CMOS_REG_RTC_SEC));
    time->tm_min = from_bcd(cmos_read(CMOS_REG_RTC_MIN));
    time->tm_hour = from_bcd(cmos_read(CMOS_REG_RTC_HOUR));
    time->tm_mday = from_bcd(cmos_read(CMOS_REG_RTC_MONTHDAY));
    time->tm_mon = from_bcd(cmos_read(CMOS_REG_RTC_MONTH));

    time->tm_year = from_bcd(cmos_read(CMOS_REG_RTC_YEAR));
    time->tm_year += year_offset;
}

void cmos_rtc_get_datetime(struct cmos_datetime *time)
{
    time->second = from_bcd(cmos_read(CMOS_REG_RTC_SEC));
    time->minute = from_bcd(cmos_read(CMOS_REG_RTC_MIN));
    time->hour = from_bcd(cmos_read(CMOS_REG_RTC_HOUR));
    time->day = from_bcd(cmos_read(CMOS_REG_RTC_MONTHDAY));
    time->month = from_bcd(cmos_read(CMOS_REG_RTC_MONTH));
    time->year = from_bcd(cmos_read(CMOS_REG_RTC_YEAR));
}

void arch_sys_get_time_epoch(struct timeval *tv)
{
    struct tm datetime;
	cmos_rtc_get_datetime_tm(&datetime);
    tv->tv_sec = tm_to_epoch(&datetime);
}