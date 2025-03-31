#include "math.h"

double fabs(double x)
{
    union {double f; unsigned long long i;} u = {x};
	u.i &= -1ULL/2;
	return u.f;
}

int isnan(double d)
{
  	union {double f; unsigned long long i;} u = {d};
  	u.i = d;
  	return (u.i == 0x7FF8000000000000ll || u.i == 0x7FF0000000000000ll || u.i == 0xfff8000000000000ll);
}

extern int isinf(double d)
{
	union {double f; unsigned long long i;} u = {d};
  	u.i = d;
	return (u.i == 0x7FF0000000000000ll ? 1 : u.i == 0xFFF0000000000000ll ? -1 : 0);
}

double floor(double x) {
    return __builtin_floor(x);
}

double ceil(double x)
{
	return __builtin_ceil(x);
}

double atan(double d)
{
	return atan2(d, 1.0);
}