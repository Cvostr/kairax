#include "stdio.h"
#include "stdio_impl.h"
#include "unistd.h"
#include "string.h"
#include "inttypes.h"

int printf(const char* restrict format, ...) {

    va_list arg_ptr;
    va_start(arg_ptr, format);
    int n = vfdprintf(STDOUT_FILENO, format, arg_ptr);
    va_end(arg_ptr);

    return n;
}

int fprintf(FILE *stream, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vfprintf(stream, format, args);
    va_end(args);

    return n;
}