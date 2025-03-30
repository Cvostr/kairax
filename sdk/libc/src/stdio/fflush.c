#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio_impl.h"

int __fflush_atexit_state = 0;
extern FILE *__stdio_file_root;

int fflush(FILE *stream)
{
    int tmp;
    
    if (stream)
        mtx_lock(&stream->_lock);

    tmp = fflush_unlocked(stream);
    
    if (stream)
        mtx_unlock(&stream->_lock);
    
    return tmp;
}

int fflush_unlocked(FILE *stream)
{
    ssize_t res;

    if (stream == NULL) 
    {
        fflush(stdout);
        
        // Выполнить fflush для всех файлов в цепочке
        FILE* cur = __stdio_file_root;

        while (cur != NULL)
        {
            if ((res = fflush_unlocked(cur)) != 0) 
            {
                return res;
            }

            cur = cur->_next;
        }

        return 0;
    }

    if ((stream->_flags & FSTREAM_INPUT) == FSTREAM_INPUT) {
        // todo : implement
    } else if (stream->_buf_pos > 0) {
        res = write(stream->_fileno, stream->_buffer, stream->_buf_pos);
        if (res == -1 || res != stream->_buf_pos) {
            stream->_flags = FSTREAM_ERROR;
            return EOF;
        }

        stream->_buf_pos = 0;
    }
    
	return 0;
}

void __fflush_all()
{
    fflush(NULL);
}

void __fflush_atexit()
{
    if (__fflush_atexit_state == 0)
    {
        atexit(__fflush_all);
        __fflush_atexit_state = 1;
    }
}