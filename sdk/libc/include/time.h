#ifndef _TIME_H_
#define _TIME_H_

#include "sys/time.h"
#include "sys/types.h"

int isleap(int year);

struct tm* gmtime(const time_t* t);
struct tm* gmtime_r(const time_t* t, struct tm* r);

time_t time(time_t *t);

#endif