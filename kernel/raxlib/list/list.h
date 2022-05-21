#ifndef _LIST_H
#define _LIST_H

typedef struct {
    void* element;
    struct list_node_t* next;
    struct list_node_t* prev;
} list_node_t;

typedef struct{
    int size;
    list_node_t* head;
    list_node_t* tail;
} list_t;


list_t* create_list();

void list_add(list_t* list, void* element);

void* list_get(list_t* list, unsigned int i);

void* list_head(list_t* list);

void* list_tail(list_t* list);

#endif