#include "interrupts/handle/handler.h"
#include "proc/thread_scheduler.h"
#include "stdio.h"
#include "interrupts/pic.h"
#include "list/list.h"
#include "memory/mem_layout.h"
#include "string.h"
#include "cpu/gdt.h"
#include "mem/pmm.h"
#include "memory/paging.h"
#include "x64_context.h"
#include "cpu/msr.h"
#include "cpu/cpu_local_x64.h"

extern void scheduler_yield_entry();
extern void scheduler_exit(thread_frame_t* ctx);
int scheduler_enabled = 0;

void scheduler_yield()
{
    // Выключение прерываний
    disable_interrupts();

    scheduler_yield_entry();
}

void scheduler_from_killed()
{
    // Выключение прерываний
    disable_interrupts();

    cpu_set_current_thread(NULL);

    // Переход в планировщик
    scheduler_handler(NULL);

    // Не должны сюда попасть
}

// frame может быть NULL
// Если это так, то мы сюда попали из убитого потока
void scheduler_handler(thread_frame_t* frame)
{
    if (!scheduler_enabled) {
        return;
    }

    struct thread* previous_thread = cpu_get_current_thread();

    // Сохранить состояние 
    if (previous_thread != NULL && frame) {

        // Сохранить указатель на контекст
        previous_thread->context = frame;

        // Если процесс не блокировался - сменить состояние
        if (previous_thread->state == STATE_RUNNING)
            previous_thread->state = STATE_RUNNABLE;

        if (previous_thread->is_userspace) {
            // Сохранить указатель на вершину стека пользователя
            previous_thread->stack_ptr = get_user_stack_ptr();
        }
    }

    // Найти следующий поток
    struct thread* new_thread = scheduler_get_next_runnable_thread();
    new_thread->state = STATE_RUNNING;

    // Получить данные процесса, с которым связан поток
    struct process* process = new_thread->process;
    thread_frame_t* thread_frame = (thread_frame_t*)new_thread->context;

    // Разрешить прерывания
    thread_frame->rflags |= 0x200;
    if (new_thread->is_userspace) {
        // Обновить данные об указателях на стек
        cpu_set_kernel_stack(new_thread->kernel_stack_ptr + PAGE_SIZE);
        tss_set_rsp0((uintptr_t)new_thread->kernel_stack_ptr + PAGE_SIZE);
        set_user_stack_ptr(new_thread->stack_ptr);

        if (new_thread->tls != NULL) {
            // TLS, обязательно конец памяти
            cpu_set_fs_base(new_thread->tls + process->tls_size);
        }
    }

    // Сохранить указатель на новый поток в локальной структуре ядра
    cpu_set_current_thread(new_thread);

    // Заменить таблицу виртуальной памяти процесса
    switch_pml4(V2P(process->vmemory_table->arch_table));

    scheduler_exit(new_thread->context);
}

void scheduler_start()
{
    scheduler_enabled = 1;
}