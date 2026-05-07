#ifndef _LIST_H
#define _LIST_H

struct list_node {
    void* element;
    struct list_node* next;
    struct list_node* prev;
};

typedef struct{
    int size;
    struct list_node* head;
    struct list_node* tail;
} list;


list* create_list();

void list_add(list* list, void* element);

void* list_get(list* list, unsigned int i);
void* list_head(list* list);
void* list_tail(list* list);

#endif