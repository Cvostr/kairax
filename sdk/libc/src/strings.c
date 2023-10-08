#include "strings.h"

int strcasecmp(const char *__s1, const char *__s2)
{
    unsigned int  x2;
    unsigned int  x1;

    while (1) {
        x2 = *__s2 - 'A';
        x1 = *__s1 - 'A'; 

        if (x2 < 26) {
            x2 += 32;
        }
        
        if (x1 < 26) {
            x1 += 32;
        }
        
        __s1++; __s2++;
        if (x2 != x1)
            break;

        if (x1 == (unsigned int)-'A')
            break;
    }

    return x1 - x2;
}

int strncasecmp(const char *__s1, const char *__s2, size_t __n)
{
    unsigned int  x2;
    unsigned int  x1;
    const char  *end = __s1 + __n;

    while (1) {
        if (__s1 >= end) {
            return 0;
        }

        x2 = *__s2 - 'A';
        x1 = *__s1 - 'A'; 

        if (x2 < 26) {
            x2 += 32;
        }
        
        if (x1 < 26) {
            x1 += 32;
        }
        
        __s1++; __s2++;
        if (x2 != x1)
            break;

        if (x1 == (unsigned int)-'A')
            break;
    }

    return x1 - x2;
}