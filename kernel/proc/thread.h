#ifndef _THREAD_H
#define _THREAD_H

#include "kairax/types.h"
#include "process.h"

#define STACK_SIZE (4096 * 2)
#define STACK_MAX_SIZE 65536

struct thread {
    // Тип объекта
    int                 type;
    // ID потока
    pid_t               id;
    // Состояние
    int                 state;
    // Код выхода
    int                 code;
    // Имя потока
    char                name[PROCESS_NAME_MAX_LEN];
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
    int                 timeslice;
    sigset_t            pending_signals;
    // Следующий поток при блокировке на семафоре
    struct thread*      prev;
    struct thread*      next;

#ifdef X86_64
    uint8_t*             fpu_context;
#endif
};

struct  aux_pair;

struct main_thread_create_info {
    struct  aux_pair* auxv;
    size_t  aux_size;
    int argc;
    char* argv;
    char* envp;
};

struct thread* new_thread(struct process* process);

struct thread* create_kthread(struct process* process, void (*function)(void), void* arg);
struct thread* create_thread(struct process* process, void* entry, void* arg1, int stack_size, struct main_thread_create_info* info);
void thread_recreate_on_execve(struct thread* thread, struct main_thread_create_info* info);

void thread_become_zombie(struct thread* thread);
void thread_destroy(struct thread* thread);

int thread_send_signal(struct thread* thread, int signal);

#endif