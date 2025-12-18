#include "err.h"
#include <stdlib.h>
#include "errno.h"
#include "stdio.h"
#include "string.h"

void err(int eval, const char* fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    vwarn(fmt, args);
    va_end(args);
    exit(eval);
}

void errx(int eval, const char* fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    vwarnx(fmt, args);
    va_end(args);
    exit(eval);
}

void verr(int eval, const char* fmt, va_list args) 
{
    vwarn(fmt, args);
    exit(eval);
}

void warn(const char* fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    vfdprintf(2, fmt, args);
    fdprintf(2, ": %s\n", strerror(errno));
    va_end(args);
}

void vwarn(const char* fmt, va_list args) 
{
    vfdprintf(2, fmt, args);
    fdprintf(2, ": %s\n", strerror(errno));
}

void vwarnx(const char *fmt, va_list args)
{
    vfdprintf(2, fmt, args);
}