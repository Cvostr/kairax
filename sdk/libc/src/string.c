#include "string.h"
#include "stdlib.h"

void *memcpy (void *__restrict __dest, const void *__restrict __src, size_t __n)
{
    char* temp_dst = __dest;
    char* temp_src = (char*) __src;

    while(__n--)
		*(temp_dst++) = *(temp_src++);

    return __dest;
}

void *memmove (void *__dest, const void *__src, size_t __n)
{
    u_char* src = (u_char*) __src;
	u_char* dest = (u_char*) __dest;
    size_t i = 0;

	if (src == dest || __n == 0)
		return __dest;

	if (dest > src && dest - src < __n) {

		for (i = __n - 1; i >= 0; i--)
			dest[i] = src[i];

		return __dest;
	}
	if (src > dest && src - dest < __n) {

		for (i = 0; i < __n; i++)
			dest[i] = src[i];

		return __dest;
	}

	memcpy(__dest, __src, __n);
	return __dest;
}

void *memset (void *__s, int __c, size_t __n)
{
    unsigned char* temp_dst = __s;
    while(__n --)
    {
        *(temp_dst++) = (unsigned char) __c;
    }

    return __s;
}

int memcmp (const void *__s1, const void *__s2, size_t __n)
{
    const unsigned char* a = (const unsigned char*) __s1;
	const unsigned char* b = (const unsigned char*) __s2;
	for (size_t i = 0; i < __n; i++)
    {
		if (a[i] < b[i]) 
        {
			return -1;
        }
		else if (b[i] < a[i]) 
        {
			return 1;
        }
	}

	return 0;
}

size_t strlen (const char *__s)
{
    size_t size = 0;

    if (__s == NULL) {
        return 0;
    }

    while(*__s++)
    {
		size++;
    }

	return size;
}

size_t strnlen (const char *__string, size_t __maxlen)
{
    size_t size = 0;

    while(*__string++ && size < __maxlen)
    {
		size++;
    }

	return size;
}

char *strcpy (char *__restrict __dest, const char *__restrict __src)
{
    return memcpy (__dest, __src, strlen (__src) + 1);
}

char *strncpy (char *__restrict __dest, const char *__restrict __src, size_t __n)
{
    size_t size = strnlen(__src, __n);
    if (size != __n)
    {
        memset (__dest + size, '\0', __n - size);
    }
    
    return memcpy (__dest, __src, size);
}

char *strcat (char *__restrict __dest, const char *__restrict __src)
{
    strcpy(__dest + strlen (__dest), __src);
    return __dest;
}

char *strncat (char *__restrict __dest, const char *__restrict __src, size_t __n)
{
    char *s = __dest;
    __dest += strlen(__dest);
    size_t ss = strnlen (__src, __n);
    __dest[ss] = '\0';
    memcpy (__dest, __src, ss);
    return s;
}

int strcmp (const char *__s1, const char *__s2)
{
    const unsigned char *s1 = (void*) __s1, *s2 = (void*) __s2;
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

int strncmp (const char *__s1, const char *__s2, size_t __n)
{
    const unsigned char *s1 =(void *)__s1, *s2=(void *)__s2;
	if (!__n--) return 0;

	for (; *s1 && *s2 && __n && *s1 == *s2 ; s1++, s2++, __n--);

	return *s1 - *s2;
}

char *strdup (const char *__s)
{
    size_t strsize = strlen(__s);
    char* str = malloc(strsize + 1);

    if (str == NULL) {
        return NULL;
    }

    str[strsize] = '\0';

    return memcpy(str, __s, strsize);
}

char *strndup (const char *__string, size_t __n)
{
    char* str = malloc(__n + 1);
    str[__n] = '\0';

    if (str == NULL) {
        return NULL;
    }

    return memcpy(str, __string, __n);
}

char *strchr (const char *__s, int __c)
{
    while(*__s) 
    {
		if (*__s == __c) {
			return (char *)__s;
        }

		__s++;
	}
	return NULL;
}

char *strrchr(const char* __s, int __c)
{
	char* s = (char*) __s;
	char* result = 0;

  	do {
    	if (*s == __c)
      		result = (char*) s;

  	} while (*s++);
  	
	return (result);
}

char *strstr(const char* __s, const char* __subs)
{
    unsigned char* str = (unsigned char*) __s;

    size_t substrl = strlen(__subs);

    while (*str) {
        if (memcmp(str, __subs, substrl) == 0) {
            return str;
        }

        str++;
    }

    return 0;
}

char* strrev(char *str)
{
    for (size_t i = 0, j = strlen(str) - 1; i < j; i ++, j --) {
        char c = str[i];
        str[i] = str[j];
        str[j] = c;
    }
}

size_t strspn(const char* dest, const char* src)
{
    size_t len = 0;
    char* p = (char*) dest;
    char* s;

    while (*p) 
    {
        s = (char*) src;

        // Попытаться найти символ в src
        while (*s && (*p != *s)) 
        {
            s ++;
        }

        if (*s) {
            len++;
        } else {
            break;
        }

        p++;
    }

    return len;
}

size_t strcspn(const char* dest, const char* src)
{
    size_t len = 0;
    char* p = (char*) dest;
    int i = 0;

    while (*p) 
    {
        for (i = 0; src[i]; i ++)
        {
            if (*p == src[i]) 
            {
                return len;
            }
        }

        len ++;
        p ++;
    }

    return len;
}