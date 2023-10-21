#include "stdio.h"
#include "stdio_impl.h"
#include "string.h"
#include "inttypes.h"

static int strwrite(void* arg, const void *ptr, size_t size) {

    struct str_data* strdat = arg;
    size_t tmp = strdat->size - strdat->len;

    if (tmp > 0) {

        size_t len = size;
        
        if (len > tmp) {
            len = tmp;
        }
        
        if (strdat->str) {
            memcpy(strdat->str + strdat->len, ptr, len);
            strdat->str[strdat->len + len] = 0;
        }

        strdat->len += len;
    }

    return size;
}

int vsnprintf(char* str, size_t size, const char *format, va_list arg_ptr)
{
    struct str_data sd = { str, 0, size };
    struct arg_printf ap = { &sd, strwrite };

    int n = printf_generic(&ap, format, arg_ptr);

    if (str && size && n >= 0) {
        if (((size_t) n >= size)) 
            str[size - 1] = 0;
        else 
            str[n] = 0;
    }

    return n;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, format);

    int n = vsnprintf(str, size, format, arg_ptr);
    
    va_end (arg_ptr);

    return n;
}

int sprintf(char *str, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsprintf(str, format, args);
    va_end (args);

    return n;
}

int vsprintf(char *str, const char *format, va_list args)
{
    return vsnprintf(str, (size_t)-1 - (uintptr_t)str, format, args);
}