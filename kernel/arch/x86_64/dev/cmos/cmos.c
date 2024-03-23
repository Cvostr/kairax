#include "cmos.h"
#include "io.h"

// 0x59 -> 59
unsigned short cmos_format(unsigned short value, int is_bcd)
{
    if (is_bcd) {
        return (value / 16) * 10 + (value & 0xf);
    }

    return value;
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

uint8_t cmos_get_register_b() {
    return cmos_read(0xb);
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
    while (cmos_is_update());

    int cmos_bcd_format = !(cmos_get_register_b() & 0x04);
    
    time->tm_sec = cmos_format(cmos_read(CMOS_REG_RTC_SEC), cmos_bcd_format);
    time->tm_min = cmos_format(cmos_read(CMOS_REG_RTC_MIN), cmos_bcd_format);
    time->tm_hour = cmos_format(cmos_read(CMOS_REG_RTC_HOUR), cmos_bcd_format);
    time->tm_mday = cmos_format(cmos_read(CMOS_REG_RTC_MONTHDAY), cmos_bcd_format);
    time->tm_mon = cmos_format(cmos_read(CMOS_REG_RTC_MONTH), cmos_bcd_format) - 1; // 0 - 11
    time->tm_year = cmos_format(cmos_read(CMOS_REG_RTC_YEAR), cmos_bcd_format);
    int century = cmos_format(cmos_read(CMOS_REG_RTC_CENTURY), cmos_bcd_format);
    
    int year_offset = century * 100 - 1900;
    time->tm_year += year_offset;
}

void cmos_rtc_set_datetime_tm(struct tm* time)
{
    // TODO : FIX
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

void arch_sys_set_time_epoch(const struct timeval *tv)
{
    struct tm datetime;
    epoch_to_tm(tv, &datetime);
    // TODO: implement
    //cmos_rtc_set_datetime_tm(&datetime);
}