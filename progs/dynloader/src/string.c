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

void *memcpy (void *__restrict __dest, const void *__restrict __src, size_t __n)
{
    char* temp_dst = __dest;
    char* temp_src = __src;

    while(__n--)
		*(temp_dst++) = *(temp_src++);

    return __dest;
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

size_t strlen (const char *__s)
{
    size_t size = 0;

    while(*__s++)
    {
		size++;
    }

	return size;
}

char *strcpy (char *__restrict __dest, const char *__restrict __src)
{
    return memcpy (__dest, __src, strlen (__src) + 1);
}