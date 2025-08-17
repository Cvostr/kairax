#ifndef _TASKLET_H
#define _TASKLET_H

#include "kairax/types.h"

#define TASKLET_STATE_INIT      0
#define TASKLET_STATE_SCHED     1
#define TASKLET_STATE_RUNNING   2

struct tasklet_list {
    struct tasklet* head;
    struct tasklet** tail;
};

struct tasklet {

    struct tasklet* next;
    int state;

    void (*func)(void*);
    void* data; 
};

#define DECLARE_TASKLET(name, func, data) \
struct tasklet name = { NULL, 0, func, data }

/// @brief Инициализировать тасклет функцией и аргументом
/// @param tasklet 
/// @param func 
/// @param data 
void tasklet_init(struct tasklet* tasklet, void (*func)(void*), void* data);
/// @brief Запланировать тасклет на выполнение на текущем процессоре (ядре)
/// @param tasklet 
void tasklet_schedule(struct tasklet* tasklet);

// Методы для внутреннего использования ядром
void tasklet_schedule_generic(struct tasklet_list* list, struct tasklet* tasklet);
struct tasklet_list* new_tasklet_list();
void tasklet_list_execute(struct tasklet_list* list);
void* create_tasklet_thread();

#endif