#include "list.h"
#include "stdlib.h"

list* create_list()
{
    list* result = malloc(sizeof(list));
    result->size = 0;
    result->head = NULL;
    result->tail = NULL;
    return result;
}

void list_add(list* list, void* element)
{
    struct list_node* new_node = malloc(sizeof(struct list_node));
    new_node->element = element;
    new_node->next = NULL;

    if(!list->head){
        list->head = new_node;
    }else{
        list->tail->next = new_node;
        new_node->prev = list->tail;
    }
    list->tail = new_node;
    list->size++;
}

void* list_get(list* list, unsigned int i){
    struct list_node* current = list->head;
    for(unsigned int _i = 0; _i < i; _i++){
        current = current->next;
    }
    return current->element;
}

void* list_head(list* list)
{
    return list->head->element;
}

void* list_tail(list* list)
{
    return list->tail->element;    
}

void list_unlink(list* list, struct list_node* node)
{
    struct list_node* prev = node->prev;
    struct list_node* next = node->next;

    if (prev) {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }

    if (list->head == node) {
        list->head = next;
        if (list->head) list->head->prev = NULL;
    }

    if (list->tail == node) {
        list->tail = prev;
        if (list->tail) list->tail->next = NULL;
    }

    list->size--;
}

void* list_dequeue(list * list) 
{
    if (list == NULL)
        return NULL;
        
	if (list->head == NULL) 
        return NULL;

	struct list_node* node = list->head;
    void* elem = node->element;
	list_unlink(list, node);
    free(node);
	return elem;
}

void* list_pop(list * list) 
{
    if (list == NULL)
        return NULL;
        
	if (list->tail == NULL) 
        return NULL;

	struct list_node* node = list->tail;
    void* elem = node->element;
	list_unlink(list, node);
    free(node);
	return elem;
}