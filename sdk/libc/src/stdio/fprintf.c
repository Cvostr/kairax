#include "stdio.h"
#include "stdio_impl.h"
#include "string.h"

static int _fwrite(void* arg, const void *ptr, size_t size) {
    char temp[130];
    int fd = (int) (long) arg;
    temp[0] = 'w';
    temp[1] = size;
    memcpy(temp + 2, ptr, size);
    fwrite(temp, 1, size + 2, (FILE*) arg);

    return size;
}

int vfprintf(FILE *stream, const char *format, va_list args)
{
    struct arg_printf ap = {(void*) (long) stream, _fwrite};
    return printf_generic(&ap, format, args);
}