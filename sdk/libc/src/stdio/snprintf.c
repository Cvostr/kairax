#include "stdio.h"
#include "stdio_impl.h"
#include "unistd.h"
#include "string.h"

char temp[130];

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

static int fdwrite(void* arg, const void *ptr, size_t size) {

    int fd = (int) (long) arg;
    temp[0] = 'w';
    temp[1] = size;
    memcpy(temp + 2, ptr, size);
    write(fd, temp, size);

    return size;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, format);

    int n = vsnprintf(str, size, format, arg_ptr);
    
    va_end (arg_ptr);

    return n;
}

int vsnprintf(char* str, size_t size, const char *format, va_list arg_ptr)
{
    struct str_data sd = { str, 0, size ? size - 1 : 0 };
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

int vfprintf(FILE *stream, const char *format, va_list args)
{
    struct arg_printf ap = {(void*) (long) stream->_fileno, fdwrite};
    return printf_generic(&ap, format, args);
}



int fdprintf(int fd, const char *format, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, format);
    int n = vfdprintf(fd, format, arg_ptr);
    va_end(arg_ptr);

    return n;
}

int vfdprintf(int fd, const char *format, va_list arg_ptr)
{
    struct arg_printf ap = {(void*) (long) fd, fdwrite};
    return printf_generic(&ap, format, arg_ptr);
}