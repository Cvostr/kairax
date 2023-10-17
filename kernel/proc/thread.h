#ifndef _THREAD_H
#define _THREAD_H

#include "types.h"
#include "process.h"
#include "elf64/elf64.h"

#define STACK_SIZE 4096

enum thread_state {
    THREAD_RUNNING                  = 0,    // Работает
    THREAD_RUNNABLE                 = 1,
    THREAD_INTERRUPTIBLE_SLEEP      = 2,    // В ожидании ресурса
    THREAD_UNINTERRUPTIBLE_SLEEP    = 3,    // В ожидании ресурса внутри системного вызова
    THREAD_ZOMBIE                   = 4     // Завершен, но не обработан родителем
};

struct thread {
    char                name[32];
    // ID потока
    pid_t               id;
    // Адрес вершины стека пользователя
    void*               stack_ptr;
    // Адрес вершины стека ядра
    void*               kernel_stack_ptr;
    // Адрес локальной памяти потока (TLS)
    void*               tls;
    // Состояние
    enum thread_state   state;
    // Указатель на объект процесса
    struct process*     process;
    // Указатель на сохраненные данные контекста
    void*               context;
    int                 is_userspace;
    void*               wait_handle;
};

struct main_thread_create_info {
    struct  aux_pair* auxv;
    size_t  aux_size;
    int argc;
    char* argv;
};

struct thread* new_thread(struct process* process);

struct thread* create_kthread(struct process* process, void (*function)(void));

struct thread* create_thread(struct process* process, void* entry, void* arg1, size_t stack_size, struct main_thread_create_info* info);

#endif