#ifndef _LIST_H
#define _LIST_H

typedef struct {
    void* element;
    struct list_node* next;
    struct list_node* prev;
}list_node;

typedef struct{
    int size;
    list_node* head;
    list_node* tail;
}list;


list* create_list();

void list_add(list* list, void* element);

void* list_get(list* list, unsigned int i);

void* list_head(list* list);

void* list_tail(list* list);

#endif