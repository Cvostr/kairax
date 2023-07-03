#include "interrupts/handle/handler.h"
#include "thread_scheduler.h"
#include "stdio.h"
#include "interrupts/pic.h"
#include "list/list.h"
#include "memory/mem_layout.h"
#include "string.h"
#include "cpu/gdt.h"
#include "kernel_stack.h"
#include "mem/pmm.h"
#include "memory/paging.h"

list_t* threads_list;
int curr_thread = 0;

thread_t* prev_thread = NULL;

void scheduler_add_thread(thread_t* thread)
{
    list_add(threads_list, thread);
}

void scheduler_remove_thread(thread_t* thread)
{
    list_remove(threads_list, thread);
}

void scheduler_remove_process_threads(process_t* process)
{
    for (unsigned int i = 0; i < process->threads->size; i ++) {
        scheduler_remove_thread(list_get(process->threads, 0));
    }
}

thread_t* scheduler_get_current_thread()
{
    return prev_thread;
}

void* scheduler_handler(thread_frame_t* frame)
{
    int is_from_interrupt = 1;
    // Сохранить состояние 
    if(prev_thread != NULL) {
        prev_thread->context = frame;
        //memcpy(&prev_thread->context, frame, sizeof(thread_frame_t));
        if (prev_thread->is_userspace) {
            // Сохранить указатель на вершину стека пользователя
            prev_thread->stack_ptr = get_user_stack_ptr();
        }

        // Если у старого потока состояние THREAD_UNINTERRUPTIBLE, значит
        // мы пришли сюда не из прерывания, а из yield
        is_from_interrupt = prev_thread->state != THREAD_UNINTERRUPTIBLE;
    }

    if(curr_thread >= threads_list->size)
        curr_thread = threads_list->size - 1;

    thread_t* new_thread = list_get(threads_list, curr_thread);
    
    if(new_thread != NULL) {
        // Получить данные процесса, с которым связан поток
        process_t* process = new_thread->process;
        thread_frame_t* thread_frame = (thread_frame_t*)new_thread->context;

        if (new_thread->is_userspace) {
            thread_frame->rflags |= 0x200;

            // Обновить данные об указателях на стек
            set_kernel_stack(new_thread->kernel_stack_ptr);
            tss_set_rsp0(new_thread->kernel_stack_ptr);
            set_user_stack_ptr(new_thread->stack_ptr);
        }

        prev_thread = new_thread;

        curr_thread++;
        if(curr_thread >= threads_list->size)
            curr_thread = 0;

        // Заменить таблицу виртуальной памяти процесса
        switch_pml4(V2P(process->vmemory_table));
    }

    if (is_from_interrupt)
	    pic_eoi(0);

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