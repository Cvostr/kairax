#include "stdio.h"
#include "stdio_impl.h"
#include "string.h"

static int _fwrite(void* arg, const void *ptr, size_t size) {
    int fd = (int) (long) arg;
    fwrite(ptr, 1, size, (FILE*) arg);

    return size;
}

int vfprintf(FILE *stream, const char *format, va_list args)
{
    struct arg_printf ap = {(void*) (long) stream, _fwrite};
    return printf_generic(&ap, format, args);
}