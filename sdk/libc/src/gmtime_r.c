#include "time.h"
#include "stddef.h"

#define SECONDS_PER_DAY (24ULL * 60 * 60)

extern const short  __spm [];

struct tm* gmtime_r(const time_t* time, struct tm* r)
{
    time_t i;
    register time_t work = *time % SECONDS_PER_DAY;
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

    for (i = 11; i && (__spm[i]>work); --i);

    r->tm_mon = i;
    r->tm_mday += work - __spm[i];
    
    return r;
}