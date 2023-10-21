#include "stdio.h"
#include "stdio_impl.h"
#include "string.h"
#include "unistd.h"

static int fdwrite(void* arg, const void *ptr, size_t size) {
    char temp[130];
    int fd = (int) (long) arg;
    temp[0] = 'w';
    temp[1] = size;
    memcpy(temp + 2, ptr, size);
    write(fd, temp, size);

    return size;
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

int vfprintf(FILE *stream, const char *format, va_list args)
{
    struct arg_printf ap = {(void*) (long) stream->_fileno, fdwrite};
    return printf_generic(&ap, format, args);
}