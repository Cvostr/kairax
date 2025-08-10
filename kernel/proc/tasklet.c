#include "tasklet.h"
#include "kairax/intctl.h"
#include "mem/kheap.h"

#define DISABLE_INTS intsenab = check_interrupts(); disable_interrupts();
#define ENABLE_INTS  if (intsenab) enable_interrupts();

int tasklet_trylock(struct tasklet* tasklet)
{
    return __sync_val_compare_and_swap(&tasklet->state, TASKLET_STATE_SCHED, TASKLET_STATE_RUNNING) == TASKLET_STATE_SCHED;
}

void tasklet_unlock(struct tasklet* tasklet)
{
    tasklet->state = TASKLET_STATE_INIT;
}

void tasklet_init(struct tasklet* tasklet, void (*func)(void*), void* data)
{
    tasklet->next = NULL;
    tasklet->state = TASKLET_STATE_INIT;
    tasklet->func = func;
    tasklet->data = data;
}

void tasklet_schedule_generic(struct tasklet_list* list, struct tasklet* tasklet)
{
    int intsenab = 0;
    DISABLE_INTS

    // Состояние - запланирован
    tasklet->state = TASKLET_STATE_SCHED;
    // Следующее звено пока отсутствует
    tasklet->next = NULL;
    // в указателе list->tail находится адрес на указатель next предыдущего тасклета
    // поэтому разыменовываем и пишем туда адрес добавляемого
    *list->tail = tasklet;
    // обновляем указатель list->tail адресом на указатель next нового тасклета
    list->tail = &tasklet->next;

    // Включаем прерывания, если были включены
    ENABLE_INTS
}

struct tasklet_list* new_tasklet_list()
{
    struct tasklet_list* tlist = kmalloc(sizeof(struct tasklet_list));
    // Первый элемент отсутствует
    tlist->head = NULL;
    // tail - указатель на указатель, в который будет записываться последний элемент
    // изначально записываем туда указатель на указатель на первый элемент
    // так как тасклетов еще нет
    tlist->tail = &tlist->head;

    return tlist;
}

void tasklet_list_execute(struct tasklet_list* list)
{
    struct tasklet* next = NULL;
    struct tasklet* cur = NULL;

    disable_interrupts();
    // Начинаем с первого элемента
    next = list->head;
    // Сброс списка (очистка)
    list->head = NULL;
    list->tail = &list->head; 
    // Возвращаем прерывания
    enable_interrupts();

    while (next != NULL)
    {
        cur = next;
        next = next->next;

        // Проверка статуса, и установка в RUNNING
        if (tasklet_trylock(cur) == TRUE)
        {
            cur->func(cur->data);
            // Сброс статуса
            tasklet_unlock(cur);
        }
    }
}