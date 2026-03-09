#ifndef _TIME_H_
#define _TIME_H_

#include <sys/cdefs.h>
#include "sys/time.h"
#include "stddef.h"

__BEGIN_DECLS

int isleap(int year);

struct tm* gmtime(const time_t* t);
struct tm* gmtime_r(const time_t* t, struct tm* r);

time_t timegm(struct tm *timeptr);

struct tm* localtime(const time_t* t) __THROW;

time_t mktime(struct tm *timeptr);

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm) __THROW;

time_t time(time_t *t);
int stime(time_t *t);

int nanosleep(const struct timespec *req, struct timespec *rem);

__END_DECLS

#endif