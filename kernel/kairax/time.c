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

#define SECONDS_PER_DAY (24ULL * 60 * 60)

int isleap(int year) 
{
    return (!(year % 4) && ((year % 100) || !(year % 400)));
}

void timespec_add(struct timespec* t1, struct timespec* t2)
{
    int64_t sum = t1->tv_nsec + t2->tv_nsec;
    if (sum > NSEC_MAX_VALUE) {
        t1->tv_nsec = sum - NSESC_IN_SEC;
        t1->tv_sec++;
    } else {
        t1->tv_nsec = sum;
    }

    t1->tv_sec += t2->tv_sec;
}

void timespec_sub(struct timespec* t1, struct timespec* t2)
{
    if (t2->tv_nsec > t1->tv_nsec) {
        // Наносекунды вычитаемого больше чем у уменьшаемого

        if (t1->tv_sec == 0) {
            // Количество секунд уменьшаемого = 0
            // Зануляем результат
            t1->tv_sec = 0;
            t1->tv_nsec = 0;
            return;            
        }

        long int diff = t2->tv_nsec - t1->tv_nsec;
        t1->tv_nsec = 999999999U - diff;
        t1->tv_sec --;
    } else {
        // Вычитаем наносекунды
        t1->tv_nsec -= t2->tv_nsec;
    }

    // Вычитаем секунды
    if (t2->tv_sec > t1->tv_sec) {
        t1->tv_sec = 0;
        t1->tv_nsec = 0;
        return;
    }

    t1->tv_sec -= t2->tv_sec;
}

int timespec_is_zero(struct timespec* t1)
{
    return (t1->tv_sec == 0 && t1->tv_nsec == 0);
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

    day += tm->tm_yday = days_in_month [tm->tm_mon] + tm->tm_mday-1 + ( isleap (tm->tm_year+1900)  &  (tm->tm_mon > 1) );

    i = 7;
    tm->tm_wday = (day + 4) % i;

    i = 24;
    day *= i;
    i = 60;

    return ((day + tm->tm_hour) * i + tm->tm_min) * i + tm->tm_sec;
}

struct tm* epoch_to_tm(const time_t* time, struct tm* r)
{
    time_t i;
    time_t work = *time % SECONDS_PER_DAY;
    r->tm_sec = work % 60;
    work /= 60;
    r->tm_min = work % 60;
    r->tm_hour = work / 60;
    work = *time / SECONDS_PER_DAY;
    r->tm_wday = (4 + work) % 7;
    for (i = 1970; ; ++i) {

        time_t k = isleap(i) ? 366 : 365;
        
        if (work >= k)
            work -= k;
        else
            break;
    }

    r->tm_year = i - 1900;
    r->tm_yday = work;

    r->tm_mday = 1;
    if (isleap(i) && (work > 58)) {
        if (work==59) 
            r->tm_mday = 2;
        work -= 1;
    }

    for (i = 11; i && (days_in_month[i]>work); --i);

    r->tm_mon = i;
    r->tm_mday += work - days_in_month[i];
    
    return r;
}