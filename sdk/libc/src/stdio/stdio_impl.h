#ifndef _STDIO_IMPL_
#define _STDIO_IMPL_

#include "stddef.h"
#include "stdarg.h"

struct arg_printf {
    void *data;
    int (*put)(void*, const void*, size_t);
};

struct str_data {
    char* str;
    size_t len;
    size_t size;
};

int printf_generic(struct arg_printf* fn, const char *format, va_list arg_ptr);

#endif