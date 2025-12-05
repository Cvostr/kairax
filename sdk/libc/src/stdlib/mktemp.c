#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "sys/time.h"
#include "sys/stat.h"
#include "stdint.h"
#include "unistd.h"
#include "fcntl.h"

#define ATTEMPTS 100

#define DIR     1
#define FILE    2
#define STAT    3

static const char __palette[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

int __mkstemp(char *template, int kind)
{
    int i, j;
    int rc;
    struct stat st;
    struct timeval tv;
    uint64_t randval;
    char* mask_start_ptr = template + strlen(template) - 6;
    if (mask_start_ptr < template)
    {
        errno = EINVAL;
        return -1;
    }

    for (i = 0; i < 6; i ++)
    {
        if (mask_start_ptr[i] != 'X')
        {
            errno = EINVAL;
            return -1;
        }
    }

    for (i = 0; i < ATTEMPTS; i ++)
    {
        gettimeofday (&tv, NULL);
        randval = ((uint64_t) tv.tv_usec << 16) ^ tv.tv_sec;
        randval = randval ^ getpid();

        for (j = 0; j < 6; j ++)
        {
            mask_start_ptr[j] = __palette[randval % 62];
            randval /= 62;
        }

        switch (kind)
        {
        case FILE:
            rc = open(template, O_CREAT | O_RDWR | O_EXCL | O_NOFOLLOW, 0600);
            break;
        case DIR:
            rc = mkdir(template, 0700);
            break;
        case STAT:
            rc = stat(template, &st);
            if (rc <= 0)
            {
                if (errno == ENOENT)
                    return 0;
                else 
                    return -1;
            }
            continue;
        }

        if (rc >= 0 || errno != EEXIST) 
            break;
    }

    return rc;
}

int mkstemp(char *template)
{
    return __mkstemp(template, FILE);
}

char* mkdtemp(char *template)
{
    if (__mkstemp(template, DIR) < 0)
        return NULL;

    return template;
}

char* mktemp(char *template)
{
    if (__mkstemp(template, STAT) != 0)
        return NULL;
    
    return template;
}