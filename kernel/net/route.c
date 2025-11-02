#include "route.h"
#include "list/list.h"
#include "sync/spinlock.h"
#include "mem/kheap.h"
#include "string.h"
#include "kairax/errors.h"

list_t      route_table4 = {0,};

struct route4* new_route4()
{
    struct route4* route = kmalloc(sizeof(struct route4));
    memset(route, 0, sizeof(struct route4));
    return route;
}

struct route4* route4_resolve(uint32_t dest)
{
    struct list_node* current = route_table4.head;
    struct route4* route = NULL;
    struct route4* chosen = NULL;
    struct route4* default_route = NULL;

    while (current != NULL) {
        
        route = (struct route4*) current->element;

        // Сеть по умолчанию 0.0.0.0
        if (route->dest == 0) {
            default_route = route;
        }

        // Полное совпадение
        if (route->dest == dest) {
            chosen = route;
            break;
        }
        
        uint32_t masked_route = route->dest & route->netmask;
        uint32_t masked_dest = dest & route->netmask;

        if (route->dest != 0 && masked_route == masked_dest) {
            chosen = route;
        }

        // Переход на следующий элемент
        current = current->next;
    }

    // Если маршрут так и не нашли - используем маршрут по умолчанию
    if (chosen == NULL) {
        chosen = default_route;
    }

    return chosen;
}

struct route4* route4_get(uint32_t index)
{
    return list_get(&route_table4, index);
}

int route4_add(struct route4* route)
{
    struct list_node* current = route_table4.head;
    struct route4* exroute = NULL;
    struct route4* chosen = NULL;

    // Сначала проверим, что такой записи еще нет
    while (current != NULL) {
        
        exroute = (struct route4*) current->element;

        if (exroute->dest == route->dest &&
            exroute->gateway == route->gateway &&
            exroute->netmask == route->netmask &&
            exroute->interface == route->interface
        )   
        {
            return -EEXIST;
        }

        // Переход на следующий элемент
        current = current->next;
    }

    // Добавляем запись
    list_add(&route_table4, route);

    return 0;
}