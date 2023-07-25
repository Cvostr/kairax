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

list_t* threads_list;
int curr_thread = 0;

struct thread* prev_thread = NULL;
spinlock_t threads_mutex = 0;

int is_from_interrupt = 1;

extern void scheduler_yield_entry();
extern void scheduler_entry_from_killed(char* stub);

static struct cpu_local_x64 __seg_gs * const this_core = 0;

void set_kernel_stack(void* kstack_top)
{
    this_core->kernel_stack = kstack_top;
}

void* get_user_stack_ptr()
{
    return this_core->user_stack;
}

void set_user_stack_ptr(void* stack_ptr)
{
    this_core->user_stack = stack_ptr;
}

void tss_set_rsp0(uintptr_t address)
{
    this_core->tss->rsp0 = address;
}

void scheduler_yield()
{
    asm("cli");
    is_from_interrupt = 0;
    scheduler_yield_entry();
}

void scheduler_from_killed()
{
    asm("cli");
    is_from_interrupt = 0;
    scheduler_entry_from_killed(NULL);
    while (1) {
        asm volatile ("nop");
    }
}

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
    acquire_mutex(&threads_mutex);

    for (unsigned int i = 0; i < list_size(process->threads); i ++) {
        struct thread* thread = list_get(process->threads, i);
        list_remove(threads_list, thread);
    }

    release_spinlock(&threads_mutex);
}

struct thread* scheduler_get_current_thread()
{
    return prev_thread;
}

void scheduler_eoi()
{
    if (is_from_interrupt) {
	    pic_eoi(0);
    } else {
        is_from_interrupt = 1;
    }
}

// frame может быть NULL
// Если это так, то мы сюда попали из убитого потока
void* scheduler_handler(thread_frame_t* frame)
{
    if (!__sync_bool_compare_and_swap(&threads_mutex, 0, 1))
	{
        frame->rflags |= 0x200;
        scheduler_eoi();
        return frame;
	}

    // Сохранить состояние 
    if(prev_thread != NULL && frame) {

        // Сохранить указатель на контекст
        prev_thread->context = frame;

        if (prev_thread->is_userspace) {
            // Сохранить указатель на вершину стека пользователя
            prev_thread->stack_ptr = get_user_stack_ptr();
        }
    }

    curr_thread ++;
    if(curr_thread >= threads_list->size)
        curr_thread = 0;

    struct thread* new_thread = list_get(threads_list, curr_thread);
    
    if(new_thread != NULL) {
        // Получить данные процесса, с которым связан поток
        struct process* process = new_thread->process;
        thread_frame_t* thread_frame = (thread_frame_t*)new_thread->context;

        // Разрешить прерывания
        thread_frame->rflags |= 0x200;
        if (new_thread->is_userspace) {
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

    scheduler_eoi();

    release_spinlock(&threads_mutex);

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