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
    off_t fdpos = 0;
    if ((stream->_flags & FSTREAM_ERROR == FSTREAM_ERROR) || (fdpos = lseek(stream->_fileno, 0, SEEK_CUR)) == -1) {
        return -1;
    }

    ssize_t modifier = 0;
    if ((stream->_flags & FSTREAM_INPUT) == FSTREAM_INPUT) {
        modifier = -(stream->_buf_size - stream->_buf_pos);
    } else {
        modifier = stream->_buf_pos;
    }

    return fdpos + modifier;
}

int fseek(FILE *stream, long offset, int whence)
{
    fflush(stream);
    stream->_buf_pos = 0;
    stream->_buf_size = 0;
    stream->_flags &= ~FSTREAM_EOF;
    return lseek(stream->_fileno, offset, whence) == -1 ? -1 : 0;
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

void rewind(FILE *stream)
{
    fseek(stream, 0, SEEK_SET);
    clearerr(stream);
}

void clearerr(FILE *stream)
{
    mtx_lock(&stream->_lock);
    clearerr_unlocked(stream);
    mtx_unlock(&stream->_lock);
}

void clearerr_unlocked(FILE *stream)
{
    stream->_flags &= ~(FSTREAM_ERROR | FSTREAM_EOF);
}

int remove(const char* filename) 
{
    if (unlink(filename) != 0) 
    {
        if (errno == EISDIR) {
            return rmdir(filename);
        }

        return -1;
    }
    
    return 0;
}

int rename(const char *oldpath, const char *newpath)
{
    __set_errno(syscall_rename(AT_FDCWD, oldpath, AT_FDCWD, newpath));
}