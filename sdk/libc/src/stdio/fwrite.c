#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio_impl.h"

size_t fwrite(const void *src, size_t size, size_t count, FILE *f)
{
    size_t len = size * count;

    if ((f->_flags & FSTREAM_CANWRITE) == 0) {
        f->_flags |= FSTREAM_ERROR;
        return 0;
    }

    return write(f->_fileno, src, len);
}