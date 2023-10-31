#ifndef _THREAD_H
#define _THREAD_H

#include "types.h"
#include "process.h"
#include "elf64/elf64.h"

#define STACK_SIZE 4096

struct thread {
    // ID потока
    pid_t               id;
    // Состояние
    int                 state;
    // Имя потока
    char                name[30];
    // Код выхода
    int                 code;
    // Адрес вершины стека пользователя
    void*               stack_ptr;
    // Адрес вершины стека ядра
    void*               kernel_stack_ptr;
    // Адрес локальной памяти потока (TLS)
    void*               tls;
    // Указатель на объект процесса
    struct process*     process;
    // Указатель на сохраненные данные контекста
    void*               context;
    int                 is_userspace;
    void*               wait_handle;
    // Следующий поток при блокировке на семафоре
    struct thread*      next_blocked_thread;
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