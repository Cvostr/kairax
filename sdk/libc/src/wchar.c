#include "wchar.h"

size_t wcslen(const wchar_t *s)
{
    // По стандарту функция не проверяет строку на NULL
    size_t size = 0;
    while (*s++)
    {
		size++;
    }

	return size;
}

int wcscmp(const wchar_t *s1, const wchar_t *s2)
{
    wchar_t c1, c2;
    do
    {
        c1 = *s1++;
        c2 = *s2++;
        if (c1 == '\0')
	        return c1 - c2;
    }
    while (c1 == c2);

    return c1 - c2;
}

wchar_t *wcscpy(wchar_t *dest, const wchar_t *src)
{
    wchar_t *origdest = dest;
    wchar_t cur;

    do {
        cur = *src;
        *dest = cur;
        src++;
        dest++;
    } while (cur != 0);

    return origdest;
}

wchar_t* wcschr(const wchar_t *wcs, wchar_t wc)
{
    while(*wcs) 
    {
		if (*wcs == wc) 
        {
			return (wchar_t *)wcs;
        }

		wcs++;
	}

	return NULL;
}