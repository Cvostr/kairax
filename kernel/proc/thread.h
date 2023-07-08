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

struct thread {
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
    struct process*      process;
    // Указатель на сохраненные данные контекста
    void*           context;
    int             is_userspace;
};

struct thread* new_thread(struct process* process);

struct thread* create_kthread(struct process* process, void (*function)(void));

struct thread* create_thread(struct process* process, void* entry, void* arg, size_t stack_size);

#endif