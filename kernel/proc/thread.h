#ifndef _THREAD_H
#define _THREAD_H

#include "types.h"
#include "process.h"

#define STACK_SIZE 4096

enum thread_state {
    THREAD_RUNNING          = 0,
    THREAD_INTERRUPTIBLE    = 1,
    THREAD_UNINTERRUPTIBLE  = 2,
    THREAD_ZOMBIE           = 4,
    THREAD_STOPPED          = 8,
    THREAD_SWAPPING         = 16,
    THREAD_EXCLUSIVE        = 32,
    THREAD_CREATED          = 64,
    THREAD_LOADING          = 128
};

typedef struct {
    char            name[32];
    // ID потока
    uint64_t        id;
    // Адрес вершины стека пользователя
    void*           stack_ptr;
    // Адрес вершины стека ядра
    void*           kernel_stack_ptr;
    // Адрес локальной памяти потока (TLS)
    void*           tls;
    // Состояние
    int             state;
    // Указатель на объект процесса
    process_t*      process;
    // Указатель на сохраненные данные контекста
    void*           context;
    int             is_userspace;
} thread_t;

thread_t* new_thread(process_t* process);

thread_t* create_kthread(process_t* process, void (*function)(void));

thread_t* create_thread(process_t* process, void* entry, void* arg, size_t stack_size);

#endif