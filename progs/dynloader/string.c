#include "string.h"

void *memset (void *__s, int __c, uint64_t __n)
{
    unsigned char* temp_dst = __s;
    while(__n --)
    {
        *(temp_dst++) = (unsigned char) __c;
    }

    return __s;
}

int strcmp (const char *__s1, const char *__s2)
{
    const unsigned char *s1 = (const unsigned char *) __s1;
    const unsigned char *s2 = (const unsigned char *) __s2;
    unsigned char c1, c2;
    
    do
    {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if (c1 == '\0')
	        return c1 - c2;
    }
    while (c1 == c2);

    return c1 - c2;
}