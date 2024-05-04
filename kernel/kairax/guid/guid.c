#include "guid.h"
#include "drivers/char/random.h"

int guid_cmp(guid_t* a, guid_t* b)
{
    return (a->d1 == b->d1 && a->d2 == b->d2 && a->d3 == b->d3 && a->d4 == b->d4 && a->d5 == b->d5);
}

int guid_is_zero(guid_t* a)
{
    return (a->d1 == 0 && a->d2 == 0 && a->d3 == 0 && a->d4 == 0 && a->d5 == 0);
}

void guid_generate(guid_t* a)
{
    a->d1 = krand();
    a->d2 = krand();
    a->d3 = krand();
    a->d4 = krand();
    a->d5 = (uint64_t) krand() << 16 | krand() >> 16;
}