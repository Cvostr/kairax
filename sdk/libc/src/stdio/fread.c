#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"

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

int fclose(FILE *stream)
{
    close(stream->_fileno);
    free(stream);
}

long ftell(FILE *stream)
{
    return lseek(stream->_fileno, 0, SEEK_CUR);
}

int fseek(FILE *stream, long offset, int whence)
{
    return lseek(stream->_fileno, offset, whence);
}

int fflush(FILE *stream)
{
	// not in kernel
	return 0;
}