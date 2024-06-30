#include "list.h"
#include "stdlib.h"

list* create_list(){
    list* result = malloc(sizeof(list));
    result->size = 0;
    result->head = NULL;
    result->tail = NULL;
    return result;
}


void list_add(list* list, void* element)
{
    list_node* new_node = malloc(sizeof(list_node));
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
    list_node* current = list->head;
    for(unsigned int _i = 0; _i < i; _i++){
        current = current->next;
    }
    return current->element;
}

void* list_head(list* list){
    return list->head->element;
}

void* list_tail(list* list){
    return list->tail->element;    
}