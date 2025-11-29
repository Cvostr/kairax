#ifndef _LIST_H
#define _LIST_H

#include "kairax/types.h"

struct list_node {
    void* element;
    struct list_node* next;
    struct list_node* prev;
};

typedef struct {
    size_t size;
    struct list_node* head;
    struct list_node* tail;
} list_t;


list_t* create_list();

void free_list(list_t* list);

void list_add(list_t* list, void* element);

struct list_node* list_get_node(list_t* list, void* element);

void list_remove(list_t* list, void* element);

size_t list_size(list_t* list);

void list_unlink(list_t* list, struct list_node* node);

void* list_get(list_t* list, size_t index);

void* list_head(list_t* list);

void* list_tail(list_t* list);

void* list_dequeue(list_t * list);
void* list_pop(list_t * list);

#endif