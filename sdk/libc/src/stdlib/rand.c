#include "stdlib.h"

static unsigned int rand_seed = 1;

#define RAND_MAX 32768

void srand(unsigned int i) 
{ 
    rand_seed = i;
}

int rand()
{
    return rand_r(&rand_seed);
}

int rand_r(unsigned int *seed)
{
    *seed = *seed * 1103515245 + 12345;
    return (unsigned int)(*seed / 65536) % RAND_MAX;
}