#include "stdio.h"
#include "unistd.h"

size_t fread(void *restrict destv, size_t size, size_t nmemb, FILE *restrict f)
{
    size_t len = size * nmemb;
    return read(f->_fileno, destv, len);
}

size_t fwrite(const void *restrict src, size_t size, size_t nmemb, FILE *restrict f)
{
    size_t len = size * nmemb;
    return write(f->_fileno, src, len);
}