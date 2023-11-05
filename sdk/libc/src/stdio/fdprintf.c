#include "stdio.h"
#include "stdio_impl.h"
#include "string.h"
#include "unistd.h"

static int fdwrite(void* arg, const void *ptr, size_t size) {
    int fd = (int) (long) arg;
    write(fd, ptr, size);

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

