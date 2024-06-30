#include "stdio.h"
#include "stdio_impl.h"

char *fgets(char *s, int size, FILE *stream)
{
    char* res = NULL;
    mtx_lock(&stream->_lock);
    res = fgets_unlocked(s, size, stream);
    mtx_unlock(&stream->_lock);
    return res;
}

char *fgets_unlocked(char *s, int size, FILE *stream)
{
    int i = 0;
    int c = 0;
    if (size <= 0) {
        return NULL;
    }

    for (i = 0; i < size - 1; i ++) 
    {
        c = fgetc(stream);

        if (c == EOF) {
            break;
        }

        s[i] = c;

        if (c == '\n') {
            break;
        }
    }

    s[i + 1] = '\0';

    return s;
}