#include "time.h"

extern const short  __spm [];

time_t timegm(struct tm *timeptr)
{
    time_t day;
    time_t i;
    time_t years = timeptr->tm_year - 70;

    if (timeptr->tm_sec > 60) { timeptr->tm_min += timeptr->tm_sec / 60; timeptr->tm_sec %= 60; }
    if (timeptr->tm_min>60) { timeptr->tm_hour += timeptr->tm_min/60; timeptr->tm_min%=60; }
    if (timeptr->tm_hour>60) { timeptr->tm_mday += timeptr->tm_hour/60; timeptr->tm_hour%=60; }
    if (timeptr->tm_mon>12) { timeptr->tm_year += timeptr->tm_mon/12; timeptr->tm_mon%=12; }
    if (timeptr->tm_mon<0) 
        timeptr->tm_mon=0;

    while (timeptr->tm_mday>__spm[1 + timeptr->tm_mon]) {
        
        if (timeptr->tm_mon == 1 && isleap(timeptr->tm_year + 1900)) {
            if (timeptr->tm_mon == 31 + 29)
                break;
            --timeptr->tm_mday;
        }

        timeptr->tm_mday -= __spm[timeptr->tm_mon];
        ++timeptr->tm_mon;
        if (timeptr->tm_mon > 11) { 
            timeptr->tm_mon = 0;
            ++timeptr->tm_year;
        }
    }

    if (timeptr->tm_year < 70)
        return (time_t) -1;

    day  = years * 365 + (years + 1) / 4;

    day += timeptr->tm_yday = __spm [timeptr->tm_mon] + timeptr->tm_mday-1 + ( isleap (timeptr->tm_year+1900)  &  (timeptr->tm_mon > 1) );

    i = 7;
    timeptr->tm_wday = (day + 4) % i;

    i = 24;
    day *= i;
    i = 60;

    return ((day + timeptr->tm_hour) * i + timeptr->tm_min) * i + timeptr->tm_sec;
}