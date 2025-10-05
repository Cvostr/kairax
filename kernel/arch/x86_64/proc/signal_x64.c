#include "kairax/signal.h"
#include "cpu/cpu_local_x64.h"
#include "x64_context.h"
#include "memory/mem_layout.h"
#include "kairax/string.h"

void arch_signal_handler(struct thread* thr, int signum, int caller, void* frame)
{
    struct process* process = thr->process;

    uint64_t old_stackptr;
    uint64_t new_stackptr;

    struct proc_sigact* sigact = &process->sigactions[signum];

    // Переключим страницу заранее, так как будем заполнять стек данными
    if (cpu_get_current_vm_table() != process->vmemory_table && process->vmemory_table != NULL) {  
        cpu_set_current_vm_table(process->vmemory_table);
        switch_pml4(V2P(process->vmemory_table->arch_table));
    } 
    else if (process->vmemory_table == NULL) {
        // Процесс завершается
        return;
    }

    printk("Caller %i handler %p\n", caller, sigact->handler);

    if (caller == CALLER_SYSCALL)
    {
        syscall_frame_t* syscall_frame = frame;

        old_stackptr = this_core->user_stack;
        new_stackptr = old_stackptr;

        new_stackptr -= 128;    // red zone

        // Надо также выровнять до 16 байт вниз
        new_stackptr -= new_stackptr % 16;

        new_stackptr -= sizeof(thread_frame_t);
        // Указатель, куда сохраним старый фрейм
        thread_frame_t* save_frame = new_stackptr;
        save_frame->rax = syscall_frame->rax;
        save_frame->rbx = syscall_frame->rbx;
        save_frame->rdx = syscall_frame->rdx;
        save_frame->rdi = syscall_frame->rdi;
        save_frame->rsi = syscall_frame->rsi;

        save_frame->rip = syscall_frame->rcx;
        
        save_frame->rsp = old_stackptr;
        save_frame->rbp = syscall_frame->rbp;

        save_frame->r8 = syscall_frame->r8;
        save_frame->r9 = syscall_frame->r9;
        save_frame->r10 = syscall_frame->r10;
        save_frame->r11 = syscall_frame->r11;
        save_frame->r12 = syscall_frame->r12;
        save_frame->r13 = syscall_frame->r13;
        save_frame->r14 = syscall_frame->r14;
        save_frame->r15 = syscall_frame->r15;

        new_stackptr -= 8;

        // Заполняем возврат
        syscall_frame->rdi = signum;
        syscall_frame->rcx = sigact->handler;

        this_core->user_stack = new_stackptr;
    }

    //printk("Handlers are not implemented\n");
}