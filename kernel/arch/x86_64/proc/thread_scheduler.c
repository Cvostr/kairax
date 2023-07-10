#include "interrupts/handle/handler.h"
#include "proc/thread_scheduler.h"
#include "stdio.h"
#include "interrupts/pic.h"
#include "list/list.h"
#include "memory/mem_layout.h"
#include "string.h"
#include "cpu/gdt.h"
#include "kernel_stack.h"
#include "mem/pmm.h"
#include "memory/paging.h"
#include "x64_context.h"
#include "cpu/msr.h"

list_t* threads_list;
int curr_thread = 0;

struct thread* prev_thread = NULL;
spinlock_t threads_mutex;

void scheduler_add_thread(struct thread* thread)
{
    acquire_mutex(&threads_mutex);
    list_add(threads_list, thread);
    release_spinlock(&threads_mutex);
}

void scheduler_remove_thread(struct thread* thread)
{
    acquire_mutex(&threads_mutex);
    list_remove(threads_list, thread);
    release_spinlock(&threads_mutex);
}

void scheduler_remove_process_threads(struct process* process)
{
    for (unsigned int i = 0; i < list_size(process->threads); i ++) {
        struct thread* thread = list_get(process->threads, i);
        scheduler_remove_thread(thread);
    }
}

struct thread* scheduler_get_current_thread()
{
    return prev_thread;
}

void* scheduler_handler(thread_frame_t* frame)
{
    int is_from_interrupt = 1;
    // Сохранить состояние 
    if(prev_thread != NULL) {

        // Сохранить указатель на контекст
        prev_thread->context = frame;

        if (prev_thread->is_userspace) {
            // Сохранить указатель на вершину стека пользователя
            prev_thread->stack_ptr = get_user_stack_ptr();
        }

        // Если у старого потока состояние THREAD_UNINTERRUPTIBLE, значит
        // мы пришли сюда не из прерывания, а из yield
        is_from_interrupt = prev_thread->state != THREAD_UNINTERRUPTIBLE;
    }

    curr_thread ++;
    if(curr_thread >= threads_list->size)
        curr_thread = 0;

    struct thread* new_thread = list_get(threads_list, curr_thread);
    
    if(new_thread != NULL) {
        // Получить данные процесса, с которым связан поток
        struct process* process = new_thread->process;
        thread_frame_t* thread_frame = (thread_frame_t*)new_thread->context;

        if (new_thread->is_userspace) {
            thread_frame->rflags |= 0x200;

            // Обновить данные об указателях на стек
            set_kernel_stack(new_thread->kernel_stack_ptr);
            tss_set_rsp0((uintptr_t)new_thread->kernel_stack_ptr);
            set_user_stack_ptr(new_thread->stack_ptr);

            if (new_thread->tls != NULL) {
                // TLS, обязательно конец памяти
                cpu_set_fs_base(new_thread->tls + process->tls_size);
            }
        }

        prev_thread = new_thread;

        // Заменить таблицу виртуальной памяти процесса
        switch_pml4(V2P(process->vmemory_table));
    } else {
        new_thread = prev_thread;
    }

    if (is_from_interrupt) {
	    pic_eoi(0);
    }

    return new_thread->context;
}

void init_scheduler()
{
    threads_list = create_list();
}

void scheduler_start()
{
    pic_unmask(0x20);
}