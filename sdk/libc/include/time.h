#ifndef _TIME_H_
#define _TIME_H_

#include <sys/cdefs.h>
#include "sys/time.h"

__BEGIN_DECLS

int isleap(int year);

struct tm* gmtime(const time_t* t);
struct tm* gmtime_r(const time_t* t, struct tm* r);

time_t timegm(struct tm *timeptr);

time_t mktime(struct tm *timeptr);

time_t time(time_t *t);
int stime(time_t *t);

int nanosleep(const struct timespec *req, struct timespec *rem);

__END_DECLS

#endif