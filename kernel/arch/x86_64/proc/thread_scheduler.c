#include "interrupts/handle/handler.h"
#include "thread_scheduler.h"
#include "stdio.h"
#include "interrupts/pic.h"
#include "list/list.h"
#include "memory/hh_offset.h"
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
    //switch_pml4(get_kernel_pml4());
    //uint64_t* ripn = (uint64_t*)((uintptr_t)frame - 8);
    //printf("%i %i", V2P(frame), V2P(*ripn));
    if(prev_thread != NULL){
        memcpy(&prev_thread->context, frame, sizeof(thread_frame_t));
    }
    
    thread_t* new_thread = list_get(threads_list, curr_thread);
    //printf(" %i", V2P(new_thread));
    if(new_thread != NULL){
        memcpy(frame, &new_thread->context, sizeof(thread_frame_t));
        prev_thread = new_thread;

        curr_thread++;
        if(curr_thread >= threads_list->size)
            curr_thread = 0;

        process_t* process = new_thread->process;
        if(process != NULL){
            switch_pml4(process->pml4);
        }else{
            //switch_pml4(get_kernel_pml4());
        }
        //printf(" %i", 223);
    }

	pic_eoi(0);
    //printf(" %i", 223);
}

void init_scheduler(){
    threads_list = P2V(create_list());
}

void start_scheduler(){
    pic_unmask(0x20);
}