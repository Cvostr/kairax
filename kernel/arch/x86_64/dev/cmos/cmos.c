#include "cmos.h"
#include "io.h"

// 0x59 -> 59
unsigned short from_bcd(unsigned short value)
{
    return (value / 16) * 10 + (value & 0xf);
}

// 59 -> 0x59
unsigned short to_bcd(unsigned short value)
{
    return (value / 10) * 16 + (value % 10);
} 

uint8_t cmos_read(unsigned reg)
{
    disable_interrupts();

    outb(CMOS_ADDRESS, reg);
    uint8_t r = inb(CMOS_DATA);

    enable_interrupts();
    return r;
}

void cmos_write(unsigned reg, uint8_t val)
{
    disable_interrupts();

    outb(CMOS_ADDRESS, reg);
    outb(CMOS_DATA, val);

    enable_interrupts();
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

    //printk("CEN %i YEAR %i\n", century, from_bcd(cmos_read(CMOS_REG_RTC_YEAR)));

    time->tm_sec = from_bcd(cmos_read(CMOS_REG_RTC_SEC));
    time->tm_min = from_bcd(cmos_read(CMOS_REG_RTC_MIN));
    time->tm_hour = from_bcd(cmos_read(CMOS_REG_RTC_HOUR));
    time->tm_mday = from_bcd(cmos_read(CMOS_REG_RTC_MONTHDAY));
    time->tm_mon = from_bcd(cmos_read(CMOS_REG_RTC_MONTH));

    time->tm_year = from_bcd(cmos_read(CMOS_REG_RTC_YEAR));
    time->tm_year += year_offset;
}

void cmos_rtc_set_datetime_tm(struct tm* time)
{
    cmos_write(CMOS_REG_RTC_SEC, to_bcd(time->tm_sec));
    cmos_write(CMOS_REG_RTC_MIN, to_bcd(time->tm_min));
    cmos_write(CMOS_REG_RTC_HOUR, to_bcd(time->tm_hour));
    cmos_write(CMOS_REG_RTC_MONTHDAY, to_bcd(time->tm_mday));
    cmos_write(CMOS_REG_RTC_MONTH, to_bcd(time->tm_mon));

    int year = time->tm_year + 1900;
    int cmos_year = year % 1000;
    int century = year / 1000;

    cmos_write(CMOS_REG_RTC_YEAR, to_bcd(cmos_year));
    cmos_write(CMOS_REG_RTC_CENTURY, to_bcd(century));
}

void arch_sys_get_time_epoch(struct timeval *tv)
{
    struct tm datetime;
	cmos_rtc_get_datetime_tm(&datetime);
    tv->tv_sec = tm_to_epoch(&datetime);
}