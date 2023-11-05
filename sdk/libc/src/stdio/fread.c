#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio_impl.h"

size_t fread(void *buffer, size_t size, size_t count, FILE *f)
{
    size_t len = size * count;
    size_t read = 0;
    int rb = 0;

    if ((f->_flags & FSTREAM_CANREAD) == 0) {
        f->_flags |= FSTREAM_ERROR;
        return 0;
    }

    for (read = 0; read < len; read ++) {
        rb = fgetc(f);

        if (rb == EOF) {
            return read / size;
        } else {
            ((unsigned char*) buffer) [read] = rb;
        }
    }

    return count;
}