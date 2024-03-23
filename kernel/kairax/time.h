#ifndef _KTIME_H
#define _KTIME_H

#include "types.h"

#define NSEC_MAX_VALUE 999999999U
#define NSESC_IN_SEC   1000000000U

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

struct timeval {
    time_t tv_sec;	        /* секунды */
    suseconds_t tv_usec;	/* микросекунды */
};

struct timespec {
	time_t tv_sec;
  	long int tv_nsec;
};

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

int isleap(int year);

time_t tm_to_epoch(struct tm *tm);
struct tm* epoch_to_tm(const time_t* time, struct tm* r);

void timespec_add(struct timespec* t1, struct timespec* t2);
void timespec_sub(struct timespec* t1, struct timespec* t2);
int timespec_is_zero(struct timespec* t1);

#endif