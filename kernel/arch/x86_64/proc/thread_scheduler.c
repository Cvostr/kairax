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

void add_thread(thread_t* thread){
    list_add(threads_list, thread);
}

void remove_thread(thread_t* thread){
    //list (threads_list, thread);
}

thread_t* get_current_thread()
{
    return prev_thread;
}

void scheduler_handler(thread_frame_t* frame){

    // Сохранить состояние 
    if(prev_thread != NULL){
        memcpy(&prev_thread->context, frame, sizeof(thread_frame_t));
    }

    thread_t* new_thread = list_get(threads_list, curr_thread);
    
    if(new_thread != NULL){
        // Заменить состояние
        memcpy(frame, &new_thread->context, sizeof(thread_frame_t));

        // Для пользовательских процессов для перехода на 3 кольцо необходимо добавить 3
        frame->cs |= (0b11 * new_thread->is_userspace);
        frame->ss |= (0b11 * new_thread->is_userspace);
        frame->rflags |= 0x200;

        prev_thread = new_thread;

        curr_thread++;
        if(curr_thread >= threads_list->size)
            curr_thread = 0;

        process_t* process = new_thread->process;
        // Заменить таблицу виртуальной памяти процесса
        if (process != NULL) {
            switch_pml4(V2P(process->pml4));
        }
    }

	pic_eoi(0);
}

void init_scheduler(){
    threads_list = create_list();
}

void start_scheduler(){
    pic_unmask(0x20);
}