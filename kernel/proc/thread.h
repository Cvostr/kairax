#ifndef _THREAD_H
#define _THREAD_H

#include "kairax/types.h"
#include "process.h"

#define DEFAULT_STACK_SIZE (4096 * 100)
#define MAX_STACK_SIZE DEFAULT_STACK_SIZE

#include "blocker.h"

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
    struct mmap_range*  stack_mapping;
    // Адрес вершины стека ядра
    void*               kernel_stack_ptr;
    // Адрес локальной памяти потока (TLS)
    void*               tls;
    struct mmap_range*  tls_mapping;
    // Указатель на объект процесса
    struct process*     process;
    // Указатель на сохраненные данные контекста
    void*               context;
    int                 is_userspace;
    struct blocker*     blocker;
    int                 timeslice;
    sigset_t            pending_signals;
    // Следующий поток при блокировке на семафоре
    struct thread**     sleep_raiser;
    int                 in_queue;
    struct thread*      prev;
    struct thread*      next;

    int                 sleep_interrupted;
    int                 balance_forbidden;
    int                 killing;
    uint32_t            cpu;
#ifdef X86_64
    uint8_t*            fpu_context;
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
void thread_set_name(struct thread* thread, const char* name);

struct thread* create_kthread(struct process* process, void (*function)(void), void* arg);
struct thread* create_thread(struct process* process, void* entry, void* arg1, int stack_size, struct main_thread_create_info* info);
void thread_recreate_on_execve(struct thread* thread, struct main_thread_create_info* info);

void thread_clear_stack_tls(struct thread* thread);
void thread_become_zombie(struct thread* thread);
void thread_destroy(struct thread* thread);

/// @brief Отправляет сигнал потоку, при необходимости будит его
/// @param thread объект потока
/// @param signal номер сигнала
/// @return 0 - при успехе, -1 при блокировке сигнала
int thread_send_signal(struct thread* thread, int signal);
/// @brief Подготавливает поток для уничтожения из другого потока. 
/// Обычно это используется при завершении процесса, чтобы безопасно остановить работу других потоков, если они были.
/// В конечном итоге state станет ZOMBIE и поток выпадет из очереди на выполнение
/// @param thread 
void thread_prepare_for_kill(struct thread* thread);

#endif