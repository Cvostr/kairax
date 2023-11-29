#ifndef _VECTOR_H
#define _VECTOR_H

struct vector {
    void* data;
    unsigned int allocated;
    unsigned int size;
};

struct vector* create_vector();
void free_vector(struct vector* vec);
void vector_add(struct vector* vec, const void* elem);
void* vector_get(struct vector* vec, unsigned int pos);

#endif