#include "time.h"

const short days_in_month[13] =
{ 
    0,
    (31),
    (31+28),
    (31+28+31),
    (31+28+31+30),
    (31+28+31+30+31),
    (31+28+31+30+31+30),
    (31+28+31+30+31+30+31),
    (31+28+31+30+31+30+31+31),
    (31+28+31+30+31+30+31+31+30),
    (31+28+31+30+31+30+31+31+30+31),
    (31+28+31+30+31+30+31+31+30+31+30),
    (31+28+31+30+31+30+31+31+30+31+30+31),
};

int isleap(int year) 
{
    return (!(year % 4) && ((year % 100) || !(year % 400)));
}

void timespec_add(struct timespec* t1, struct timespec* t2)
{
    int64_t sum = t1->tv_nsec + t2->tv_nsec;
    if (sum > NSEC_MAX_VALUE) {
        sum -= NSESC_IN_SEC;
        t1->tv_sec++;
    }

    t1->tv_sec += t2->tv_sec;
}

time_t tm_to_epoch(struct tm *tm)
{
    time_t day;
    time_t i;
    time_t years = tm->tm_year - 70;

    if (tm->tm_sec > 60) { tm->tm_min += tm->tm_sec / 60; tm->tm_sec %= 60; }
    if (tm->tm_min>60) { tm->tm_hour += tm->tm_min/60; tm->tm_min%=60; }
    if (tm->tm_hour>60) { tm->tm_mday += tm->tm_hour/60; tm->tm_hour%=60; }
    if (tm->tm_mon>12) { tm->tm_year += tm->tm_mon/12; tm->tm_mon%=12; }
    if (tm->tm_mon<0) 
        tm->tm_mon=0;

    while (tm->tm_mday>days_in_month[1 + tm->tm_mon]) {
        
        if (tm->tm_mon == 1 && isleap(tm->tm_year + 1900)) {
            if (tm->tm_mon == 31 + 29)
                break;
            --tm->tm_mday;
        }

        tm->tm_mday -= days_in_month[tm->tm_mon];
        ++tm->tm_mon;
        if (tm->tm_mon > 11) { 
            tm->tm_mon = 0;
            ++tm->tm_year;
        }
    }

    if (tm->tm_year < 70)
        return (time_t) -1;

    day  = years * 365 + (years + 1) / 4;

    /* After 2100 we have to substract 3 leap years for every 400 years
        This is not intuitive. Most mktime implementations do not support
        dates after 2059, anyway, so we might leave this out for it's
        bloat. */
    if ((years -= 131) >= 0) {
        years /= 100;
        day -= (years >> 2) * 3 + 1;
        if ((years &= 3) == 3) years--;
        day -= years;
    }

    day += tm->tm_yday = days_in_month [tm->tm_mon] + tm->tm_mday-1 + ( isleap (tm->tm_year+1900)  &  (tm->tm_mon > 1) );

    i = 7;
    tm->tm_wday = (day + 4) % i;

    i = 24;
    day *= i;
    i = 60;

    return ((day + tm->tm_hour) * i + tm->tm_min) * i + tm->tm_sec;
}