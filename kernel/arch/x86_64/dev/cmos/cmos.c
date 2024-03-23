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
unsigned short to_cmos_format(unsigned short value, int is_bcd)
{
    if (is_bcd) {
        return (value / 10) * 16 + (value % 10);
    }

    return value;
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
    return cmos_read(CMOS_STATUS_B);
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

    int is_bcd_format = !(cmos_get_register_b() & 0x04);
    
    time->tm_sec = cmos_format(cmos_read(CMOS_REG_RTC_SEC), is_bcd_format);
    time->tm_min = cmos_format(cmos_read(CMOS_REG_RTC_MIN), is_bcd_format);
    time->tm_hour = cmos_format(cmos_read(CMOS_REG_RTC_HOUR), is_bcd_format);
    time->tm_mday = cmos_format(cmos_read(CMOS_REG_RTC_MONTHDAY), is_bcd_format);
    time->tm_mon = cmos_format(cmos_read(CMOS_REG_RTC_MONTH), is_bcd_format) - 1; // 0 - 11
    time->tm_year = cmos_format(cmos_read(CMOS_REG_RTC_YEAR), is_bcd_format);
    int century = cmos_format(cmos_read(CMOS_REG_RTC_CENTURY), is_bcd_format);
    
    int year_offset = century * 100 - 1900;
    time->tm_year += year_offset;
}

void cmos_rtc_set_datetime_tm(struct tm* time)
{
    int year = time->tm_year + 1900;
    int cmos_year = year % 100;
    int century = year / 100;
    int month = time->tm_mon + 1; // tm month is 0-11

    int cmos_ctrl = cmos_get_register_b();
    int is_bcd_format = !(cmos_ctrl & 0x04);

    cmos_write(CMOS_STATUS_B, cmos_ctrl | 0x80);

    cmos_write(CMOS_REG_RTC_YEAR, to_cmos_format(cmos_year, is_bcd_format));
    cmos_write(CMOS_REG_RTC_CENTURY, to_cmos_format(century, is_bcd_format));
    cmos_write(CMOS_REG_RTC_MONTH, to_cmos_format(month, is_bcd_format));
    cmos_write(CMOS_REG_RTC_MONTHDAY, to_cmos_format(time->tm_mday, is_bcd_format));
    cmos_write(CMOS_REG_RTC_HOUR, to_cmos_format(time->tm_hour, is_bcd_format));
    cmos_write(CMOS_REG_RTC_MIN, to_cmos_format(time->tm_min, is_bcd_format));
    cmos_write(CMOS_REG_RTC_SEC, to_cmos_format(time->tm_sec, is_bcd_format));

    cmos_write(CMOS_STATUS_B, cmos_ctrl);
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
    cmos_rtc_set_datetime_tm(&datetime);
}