#include "stdio.h"
#include "stdio_impl.h"

static int sgetc(struct str_data* sd) {
    unsigned int ret = *(sd->str++);
    return ret ? (int)ret : -1;
}

static int sputc(int c, struct str_data* sd) {
    return (*(--sd->str) == c) ? c : -1;
}

int sscanf(const char *str, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsscanf(str, format, args);
    va_end (args);

    return n;
}

int vsscanf(const char *str, const char *format, va_list args)
{
    struct str_data  fdat = { (unsigned char*)str, 0, 0};
    struct arg_scanf farg = { (void*)&fdat, (int(*)(void*))sgetc, (int(*)(int,void*))sputc };

    return scanf_generic(&farg, format, args);
}

int scanf(const char *format, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, format);
    int n = vfscanf(stdin, format, arg_ptr);
    va_end (arg_ptr);
    return n;
}

int vfscanf(FILE *stream, const char *format, va_list args)
{
    //struct arg_scanf farg = { (void*)stream, (int(*)(void*))fgetc, (int(*)(int,void*))ungetc };
    //return scanf_generic(&farg, format, args);
    return 0;
}