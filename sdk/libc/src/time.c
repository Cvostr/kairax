#include "time.h"
#include "stddef.h"
#include "syscalls.h"
#include "errno.h"

const short __spm[13] =
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

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
    __set_errno(syscall_set_time_epoch(tv));
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

int stime(time_t *t)
{
    struct timeval tv;
    tv.tv_sec = *t;
    tv.tv_usec = 0;
    return settimeofday(&tv, NULL);
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    int rc = syscall_sleep(req->tv_sec, req->tv_nsec);
    __set_errno(rc);
}

struct tm* gmtime(const time_t* time)
{
    static struct tm tmtmp;
    return gmtime_r(time, &tmtmp);
}

time_t mktime(struct tm *timeptr)
{
    return timegm(timeptr);
}