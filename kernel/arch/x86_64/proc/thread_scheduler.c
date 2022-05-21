#include "interrupts/handle/handler.h"
#include "thread_scheduler.h"
#include "stdio.h"
#include "interrupts/pic.h"
#include "list/list.h"

#include "string.h"

list_t* threads_list;
int curr_thread = 0;

thread_t* prev_thread = NULL;

void add_thread(thread_t* thread){
    list_add(threads_list, thread);
}

void remove_thread(thread_t* thread){
    //list (threads_list, thread);
}

void scheduler_handler(thread_frame_t* frame){
    if(prev_thread != NULL){
        memcpy(&prev_thread->context, frame, sizeof(thread_frame_t));
    }
    
    thread_t* new_thread = list_get(threads_list, curr_thread);
    if(new_thread != NULL){
        memcpy(frame, &new_thread->context, sizeof(thread_frame_t));
        prev_thread = new_thread;

        curr_thread++;
        if(curr_thread >= threads_list->size)
            curr_thread = 0;

        process_t* process = new_thread->process;
        if(process != NULL){
            //switch_pml4(process->pml4);
        }else{
            //page_table_t* pt = (void*)0x610000;
            //switch_pml4(pt);
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