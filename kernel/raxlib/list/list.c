#include "list.h"
#include "mem/kheap.h"
#include "stddef.h"

list_t* create_list()
{
    list_t* result = kmalloc(sizeof(list_t));
    result->size = 0;
    result->head = NULL;
    result->tail = NULL;
    return result;
}

void free_list(list_t* list)
{
    struct list_node* current = list->head;
    for(unsigned int i = 0; i < list->size; i++)
    {
        struct list_node* temp = current;
        if (current) {
            current = current->next;
            kfree(temp);
        }   
    }
}

void list_add(list_t* list, void* element)
{
    struct list_node* new_node = kmalloc(sizeof(struct list_node));
    new_node->element = element;
    new_node->next = NULL;

    if (!list->head) {
        // Первого элемента не существует
        list->head = new_node;
    } else {
        list->tail->next = new_node;
        new_node->prev = list->tail;
    }

    list->tail = new_node;
    list->size++;
}

void list_remove(list_t* list, void* element)
{
    if(list == NULL)
        return;

    // Найти запись для удаления
    struct list_node* current = list->head;
    for(unsigned int i = 0; i < list->size; i++)
    {
        if (current->element == element)
            break;

        current = current->next;
    }

    list_unlink(list, current);
    kfree(current);
}

void list_unlink(list_t* list, struct list_node* node)
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
    }

    if (list->tail == node) {
        list->tail = prev;
    }

    list->size--;
}

void* list_get(list_t* list, unsigned int index)
{
    if(list == NULL)
        return NULL;

    struct list_node* current = list->head;

    for(unsigned int i = 0; i < index; i++){
        current = current->next;
    }
    
    return current->element;
}

void* list_head(list_t* list)
{
    return list->head->element;
}

void* list_tail(list_t* list)
{
    return list->tail->element;    
}