#include "list.h"
#include "mem/kheap.h"
#include "stddef.h"

list_t* create_list(){
    list_t* result = kmalloc(sizeof(list_t));
    result->size = 0;
    result->head = NULL;
    result->tail = NULL;
    return result;
}

void list_add(list_t* list, void* element){
    list_node_t* new_node = kmalloc(sizeof(list_node_t));
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

void list_remove(list_t* list, void* element){
    
}

void* list_get(list_t* list, unsigned int i){
    list_node_t* current = list->head;
    for(unsigned int _i = 0; _i < i; _i++){
        current = current->next;
    }
    return current->element;
}

void* list_head(list_t* list){
    return list->head->element;
}

void* list_tail(list_t* list){
    return list->tail->element;    
}