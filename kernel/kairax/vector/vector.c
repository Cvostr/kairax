#include "vector.h"
#include "mem/kheap.h"
#include "string.h"

#define ELEM_SIZE sizeof(void*)
#define PREALLOC 1

struct vector* create_vector()
{
    struct vector* vec = (struct vector*) kmalloc(sizeof(struct vector));
    memset(vec, 0, sizeof(struct vector));
    vec->allocated = PREALLOC;
    vec->data = kmalloc(ELEM_SIZE * vec->allocated);
    vec->size = 0;

    return vec;
}

void free_vector(struct vector* vec)
{
    kfree(vec->data);
    kfree(vec);
}

void vector_add(struct vector* vec, const void* elem)
{
    acquire_spinlock(&vec->lock);

    if(vec->allocated <= vec->size){
        void* temp = kmalloc((vec->allocated + PREALLOC) * ELEM_SIZE);
        memcpy(temp, vec->data, (vec->allocated) * ELEM_SIZE);
        kfree(vec->data);
        vec->data = temp;
        vec->allocated += PREALLOC;
    }
    
    // Скопировать элемент
    memcpy((char*) vec->data + vec->size * ELEM_SIZE, elem, ELEM_SIZE);
    vec->size += 1;

    release_spinlock(&vec->lock);
}

void* vector_get(struct vector* vec, unsigned int pos)
{
    return (char*) vec->data + pos * ELEM_SIZE;
}