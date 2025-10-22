#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include "types.h"
#include <sys/cdefs.h>

__BEGIN_DECLS

struct tm {
    int tm_sec;			
    int tm_min;			
    int tm_hour;
    int tm_mday;
    int tm_mon;	
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;

    long int tm_gmtoff;
    const char *tm_zone;
};

struct timespec {
	time_t tv_sec;
  	long int tv_nsec;
};

struct timeval {
    time_t tv_sec;	        /* секунды */
    suseconds_t tv_usec;	/* микросекунды */
};

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#include <sys/select.h>

int gettimeofday(struct timeval *tv, struct timezone *tz);
int settimeofday(const struct timeval *tv, const struct timezone *tz);

__END_DECLS

#endif