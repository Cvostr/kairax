#include "stdio.h"
#include "stdio_impl.h"

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, format);

    int n = vsnprintf(str,size,format,arg_ptr);
    
    va_end (arg_ptr);

    return n;
}

int vsnprintf(char* str, size_t size, const char *format, va_list arg_ptr)
{
    printf("vsnprintf: not implemented!");

    int n = 0;
    struct str_data sd = { str, 0, size?size-1:0 };
    /*struct arg_printf ap = { &sd, swrite };
    n=__v_printf(&ap,format,arg_ptr);
    if (str && size && n>=0) {
        if (size!=(size_t)-1 && ((size_t)n>=size)) str[size-1]=0;
        else str[n]=0;
    }*/
    return n;
}

int printf_generic(struct arg_printf* fn, const char *format, va_list arg_ptr)
{

}