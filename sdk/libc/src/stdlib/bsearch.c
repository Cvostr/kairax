#include "stdlib.h"

void *bsearch(const void *key, const void *base, size_t n, size_t size, int (*compare)(const void *, const void *))
{
    size_t left = 0;
    size_t right = n - 1;
    size_t mid;
    void *ptr;
    int res;

    while (left <= right)
    {
        mid = (left + right) / 2;
        ptr = (void *) (((const char *) base) + (mid * size));
        res = compare(ptr, key);

        if (res == 0)
        {
            return ptr;
        }
        else if (res < 0)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    return 0;
}