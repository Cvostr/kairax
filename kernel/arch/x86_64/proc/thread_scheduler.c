#include "interrupts/handle/handler.h"
#include "thread_scheduler.h"
#include "stdio.h"
#include "interrupts/pic.h"
#include "list/list.h"
#include "memory/mem_layout.h"
#include "string.h"
#include "cpu/gdt.h"

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

void scheduler_handler(thread_frame_t* frame)
{
    // Сохранить состояние 
    if(prev_thread != NULL){
        memcpy(&prev_thread->context, frame, sizeof(thread_frame_t));
    }

    if(curr_thread >= threads_list->size)
        curr_thread = threads_list->size - 1;

    thread_t* new_thread = list_get(threads_list, curr_thread);
    
    if(new_thread != NULL){
        // Заменить состояние
        memcpy(frame, &new_thread->context, sizeof(thread_frame_t));

        // Получить данные процесса, с которым связан поток
        process_t* process = new_thread->process;

        // Для пользовательских процессов для перехода на 3 кольцо необходимо добавить 3 в RPL
        frame->cs |= (0b11 * new_thread->is_userspace);
        frame->ss |= (0b11 * new_thread->is_userspace);
        frame->rflags |= 0x200;

        prev_thread = new_thread;

        curr_thread++;
        if(curr_thread >= threads_list->size)
            curr_thread = 0;

        // Заменить таблицу виртуальной памяти процесса
        if (process != NULL) {
            switch_pml4(V2P(process->pml4));
        }
    }

	pic_eoi(0);
}

void init_scheduler()
{
    threads_list = create_list();
}

void scheduler_start()
{
    pic_unmask(0x20);
}