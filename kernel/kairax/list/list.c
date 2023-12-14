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

    for (size_t i = 0; i < list->size; i++)
    {
        struct list_node* temp = current;
        if (current) {
            current = current->next;
            kfree(temp);
        }
    }

    kfree(list);
}

size_t list_size(list_t* list)
{
    return list->size;
}

void list_add(list_t* list, void* element)
{
    struct list_node* new_node = kmalloc(sizeof(struct list_node));
    new_node->element = element;
    new_node->next = NULL;
    new_node->prev = NULL;

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
    struct list_node* node = list_get_node(list, element);

    if (node) {
        list_unlink(list, node);
        kfree(node);
    }
}

struct list_node* list_get_node(list_t* list, void* element)
{
    if (list == NULL)
        return NULL;

    // Найти запись для удаления
    struct list_node* current = list->head;
    for (size_t i = 0; i < list->size; i++)
    {
        if (current->element == element) {
            
            return current;
        }

        current = current->next;
    }

    return NULL;
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

void* list_get(list_t* list, size_t index)
{
    if (list == NULL)
        return NULL;

    if (index > list->size)
        return NULL;

    struct list_node* current = list->head;

    for (size_t i = 0; i < index; i++) {
        if (current->next)
            current = current->next;
        else 
            return NULL;
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