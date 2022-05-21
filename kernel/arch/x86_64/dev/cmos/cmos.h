#ifndef _CMOS_H
#define _CMOS_H

#include "stdint.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define CMOS_REG_RTC_SEC 	    0x00
#define CMOS_REG_RTC_MIN 	    0x02
#define CMOS_REG_RTC_HOUR 	    0x04

#define CMOS_REG_RTC_WEEKDAY 	0x06
#define CMOS_REG_RTC_MONTHDAY   0x07
#define CMOS_REG_RTC_MONTH	    0x08
#define CMOS_REG_RTC_YEAR	    0x09

#define CMOS_STATUS_A           0x0A

typedef struct {
    uint16_t     year;
    uint8_t     month;
    uint8_t     day;

    uint8_t     hour;
    uint8_t     minute;
    uint8_t     second;
} cmos_datetime_t;

cmos_datetime_t cmos_rtc_get_datetime();

#endif
