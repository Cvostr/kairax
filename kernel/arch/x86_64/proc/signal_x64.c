#include "kairax/signal.h"
#include "cpu/cpu_local_x64.h"
#include "kairax/intctl.h"
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

        save_frame->rflags = syscall_frame->r11;

        save_frame->r8 = syscall_frame->r8;
        save_frame->r9 = syscall_frame->r9;
        save_frame->r10 = syscall_frame->r10;
        save_frame->r12 = syscall_frame->r12;
        save_frame->r13 = syscall_frame->r13;
        save_frame->r14 = syscall_frame->r14;
        save_frame->r15 = syscall_frame->r15;

        save_frame->cs = GDT_BASE_USER_CODE_SEG | 0b11;
        save_frame->ss = GDT_BASE_USER_DATA_SEG | 0b11;

        // Кладем в стек адрес возврата, 
        // на который будет передаваться управление после выхода из функции обработчика
        new_stackptr -= 8;
        *((uintptr_t*) new_stackptr) = process->sighandle_trampoline;

        // Заполняем возврат
        syscall_frame->rdi = signum;
        syscall_frame->rcx = sigact->handler;

        // Устанавливаем стек
        syscall_frame->rbp = new_stackptr;
        this_core->user_stack = new_stackptr;
    }
    else if (caller == CALLER_SCHEDULER)
    {
        thread_frame_t* old_frame = thr->context; 

        old_stackptr = old_frame->rsp;
        new_stackptr = old_stackptr;

        *((uint32_t*) old_stackptr) = 0;

        new_stackptr -= 128;    // red zone
        // Надо также выровнять до 16 байт вниз
        new_stackptr -= new_stackptr % 16;
        // Вычитаем размер фрейма
        new_stackptr -= sizeof(thread_frame_t);

        // Копирование старого фрейма
        memcpy(new_stackptr, old_frame, sizeof(thread_frame_t));

        // Кладем в стек адрес возврата, 
        // на который будет передаваться управление после выхода из функции обработчика
        new_stackptr -= 8;
        *((uintptr_t*) new_stackptr) = process->sighandle_trampoline;

        old_frame->rdi = signum;
        old_frame->rip = sigact->handler;

        // Устанавливаем стек
        old_frame->rsp = new_stackptr;
        old_frame->rbp = new_stackptr;

        return;
    }
    else
    {
        printk("Handler is not implemented for caller %i\n", caller);
    }
}

void arch_sigreturn()
{
    struct thread* thr = cpu_get_current_thread();

    thread_frame_t* old_frame = this_core->user_stack;

    disable_interrupts();
    scheduler_exit(old_frame);
}