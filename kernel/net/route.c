#include "route.h"
#include "list/list.h"
#include "sync/spinlock.h"

list_t      route_table4 = {0,};

struct route4* route4_resolve(uint32_t dest)
{
    struct list_node* current = route_table4.head;
    struct route4* route = NULL;
    struct route4* chosen = NULL;

    while (current != NULL) {
        
        route = (struct route4*) current->element;

        // Сеть по умолчанию 0.0.0.0
        if (route->dest == 0) {
            chosen = route;
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

    return chosen;
}

struct route4* route4_get(uint32_t index)
{
    return list_get(&route_table4, index);
}

int route4_add(struct route4* route)
{
    // todo: проверка конфликтов
    list_add(&route_table4, route);

    return 0;
}