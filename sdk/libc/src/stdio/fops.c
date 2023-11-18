#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio_impl.h"
#include "syscalls.h"
#include "fcntl.h"
#include "errno.h"

int fclose(FILE *stream)
{
    fflush(stream);
    close(stream->_fileno);
    if (stream->_buffer) {
        free(stream->_buffer);
    }
    free(stream);
}

long ftell(FILE *stream)
{
    return lseek(stream->_fileno, 0, SEEK_CUR);
}

int fseek(FILE *stream, long offset, int whence)
{
    stream->_buf_pos = 0;
    stream->_buf_size = 0;
    return lseek(stream->_fileno, offset, whence);
}

int fileno(FILE *stream)
{
    return stream->_fileno;
}

int feof(FILE *stream)
{
    return (stream->_flags & FSTREAM_EOF) == FSTREAM_EOF;
}

int ferror(FILE *stream)
{
    return (stream->_flags & FSTREAM_ERROR) == FSTREAM_ERROR;
}

int remove(const char* filename) {
    // todo : implement
    return 0;
}

int rename(const char *oldpath, const char *newpath)
{
    __set_errno(syscall_rename(AT_FDCWD, oldpath, AT_FDCWD, newpath));
}