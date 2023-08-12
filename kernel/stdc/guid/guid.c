#include "guid.h"

int guid_cmp(guid_t* a, guid_t* b){
    return (a->d1 == b->d1 && a->d2 == b->d2 && a->d3 == b->d3 && a->d4 == b->d4 && a->d5 == b->d5);
}

int guid_is_zero(guid_t* a){
    return (a->d1 == 0 && a->d2 == 0 && a->d3 == 0 && a->d4 == 0 && a->d5 == 0);
}