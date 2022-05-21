#include "cmos.h"
#include "io.h"

static uint8_t from_binary_coded_decimal(uint8_t value)
{
    return (value / 16) * 10 + (value & 0xf);
}

uint8_t cmos_read(uint32_t cmos_reg)
{
    outb(CMOS_ADDRESS, cmos_reg);
    return inb(CMOS_DATA);
}

int cmos_is_update()
{
    return cmos_read(CMOS_STATUS_A) & 0x80;
}

cmos_datetime_t cmos_rtc_get_datetime()
{
    cmos_datetime_t datetime;

    datetime.second = from_binary_coded_decimal(cmos_read(CMOS_REG_RTC_SEC));
    datetime.minute = from_binary_coded_decimal(cmos_read(CMOS_REG_RTC_MIN));
    datetime.hour = from_binary_coded_decimal(cmos_read(CMOS_REG_RTC_HOUR));
    datetime.day = from_binary_coded_decimal(cmos_read(CMOS_REG_RTC_MONTHDAY));
    datetime.month = from_binary_coded_decimal(cmos_read(CMOS_REG_RTC_MONTH));
    datetime.year = (uint16_t)from_binary_coded_decimal(cmos_read(CMOS_REG_RTC_YEAR)) + 2000;

    return datetime;
}