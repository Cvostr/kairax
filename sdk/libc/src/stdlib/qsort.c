#include "stdlib.h"

void __qs_exch(char* base, size_t offseta, size_t offsetb, size_t elemsz)
{
    char* da = base + offseta * elemsz;
    char* db = base + offsetb * elemsz;

    while (elemsz)
    {
        char temp = *da;
        *da = *db;
        *db = temp;
        da++;
        db++;
        elemsz--;
    }
}

size_t __qs_swap(char* base, size_t elemsize, size_t start, size_t end, int (*compare)(const void *, const void *))
{
    char* last = base + end * elemsize;
    size_t starti = start;
    size_t i;

    for (i = start; i < end; i ++)
    {
        if (compare(base + i * elemsize, last) < 0)
        {
            __qs_exch(base, starti, i, elemsize);
            starti++;
        }
    }

    __qs_exch(base, starti, end, elemsize);

    return starti;
}

void __qs(char* base, size_t elemsize, size_t start, size_t end, int (*compare)(const void *, const void *))
{
    if (start < end)
    {
        size_t index = __qs_swap(base, elemsize, start, end, compare);
		
        if (index > 0)
            __qs(base, elemsize, start, index - 1, compare);
        
        __qs(base, elemsize, index + 1, end, compare);
    }
}

void qsort(void *base, size_t n, size_t size, int (*compare)(const void *, const void *))
{
    __qs((char*) base, size, 0, n - 1, compare);
}