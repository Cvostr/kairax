#ifndef _VECTOR_H
#define _VECTOR_H

#include "sync/spinlock.h"

struct vector {
    void*           data;
    unsigned int    allocated;
    unsigned int    size;
    spinlock_t      lock;
};

struct vector* create_vector();
void free_vector(struct vector* vec);
void vector_add(struct vector* vec, const void* elem);
void* vector_get(struct vector* vec, unsigned int pos);

#endif