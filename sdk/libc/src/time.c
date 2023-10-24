#include "time.h"
#include "stddef.h"
#include "syscalls.h"

#define SPD (24ULL * 60 * 60)

static const short __spm[13] =
  { 0,
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

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    return syscall_get_time_epoch(tv);
}

time_t time(time_t *t)
{
    struct timeval tv;
    int rc = gettimeofday(&tv, NULL);
    if (rc < 0) {
        tv.tv_sec = -1;
    }

    if (t) {
        *t = tv.tv_sec;
    }

    return tv.tv_sec;
}

struct tm* gmtime(const time_t* time)
{
    static struct tm tmtmp;
    return gmtime_r(time, &tmtmp);
}

struct tm* gmtime_r(const time_t* time, struct tm* r)
{
    time_t i;
    register time_t work = *time % SPD;
    r->tm_sec = work % 60;
    work /= 60;
    r->tm_min = work % 60;
    r->tm_hour = work / 60;
    work = *time / SPD;
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