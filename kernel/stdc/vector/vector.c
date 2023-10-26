#include "vector.h"
#include "mem/kheap.h"
#include "string.h"

struct vector* create_vector(unsigned int elem_size)
{
    struct vector* vec = (struct vector*) kmalloc(sizeof(struct vector));
    vec->allocated = 1;
    vec->data = kmalloc(elem_size);
    vec->size = 0;
    vec->elem_size = elem_size;

    return vec;
}

void free_vector(struct vector* vec)
{
    kfree(vec->data);
    kfree(vec);
}

void vector_add(struct vector* vec, const void* elem)
{
    if(vec->allocated <= vec->size){
        void* temp = kmalloc((vec->allocated + 1) * vec->elem_size);
        memcpy(temp, vec->data, (vec->allocated) * vec->elem_size);
        kfree(vec->data);
        vec->data = temp;
        vec->allocated += 1;
    }
    
    memcpy((char*)vec->data + vec->size * vec->elem_size, elem, vec->elem_size);
    vec->size += 1;
}

void* vector_get(struct vector* vec, unsigned int pos)
{
    return (char*) vec->data + pos * vec->elem_size;
}