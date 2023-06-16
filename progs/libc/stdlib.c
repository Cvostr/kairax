#include "stdlib.h"

int abs (int i)
{
    return i < 0 ? -i : i;
}

long int labs (long int i)
{
    return i < 0 ? -i : i;
}

long long int llabs (long long int i)
{
    return i < 0 ? -i : i;
}