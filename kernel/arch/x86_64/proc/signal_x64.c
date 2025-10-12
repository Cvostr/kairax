#include "kairax/signal.h"
#include "cpu/cpu_local_x64.h"
#include "kairax/intctl.h"
#include "x64_context.h"
#include "memory/mem_layout.h"
#include "kairax/string.h"
#include "interrupts/handle/handler.h"

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

        // red zone
        new_stackptr -= 128;
        // Надо также выровнять до 16 байт вниз
        new_stackptr -= new_stackptr % 16;
        // Вычитаем размер фрейма
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

        // red zone
        new_stackptr -= 128;
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

        // Заполняем возврат
        old_frame->rdi = signum;
        old_frame->rip = sigact->handler;

        // Устанавливаем стек
        old_frame->rsp = new_stackptr;
        old_frame->rbp = new_stackptr;

        return;
    }
    else if (caller == CALLER_INTERRUPT)
    {
        interrupt_frame_t* old_frame = frame; 

        old_stackptr = old_frame->rsp;
        new_stackptr = old_stackptr;

        // red zone
        new_stackptr -= 128;
        // Надо также выровнять до 16 байт вниз
        new_stackptr -= new_stackptr % 16;
        // Вычитаем размер фрейма
        new_stackptr -= sizeof(thread_frame_t);
        // Указатель, куда сохраним старый фрейм
        thread_frame_t* save_frame = new_stackptr;
        
        // Копируем таким образом, так как структуры разные
        save_frame->rax = old_frame->rax;
        save_frame->rbx = old_frame->rbx;
        save_frame->rcx = old_frame->rcx;
        save_frame->rdx = old_frame->rdx;
        save_frame->rdi = old_frame->rdi;
        save_frame->rsi = old_frame->rsi;

        save_frame->rsp = old_frame->rsp;
        save_frame->rbp = old_frame->rbp;
        save_frame->rip = old_frame->rip;

        save_frame->r8 = old_frame->r8;
        save_frame->r9 = old_frame->r9;
        save_frame->r10 = old_frame->r10;
        save_frame->r11 = old_frame->r11;
        save_frame->r12 = old_frame->r12;
        save_frame->r13 = old_frame->r13;
        save_frame->r14 = old_frame->r14;
        save_frame->r15 = old_frame->r15;

        save_frame->rflags = old_frame->rflags;
        save_frame->ss = old_frame->ss;
        save_frame->cs = old_frame->cs;

        // Кладем в стек адрес возврата, 
        // на который будет передаваться управление после выхода из функции обработчика
        new_stackptr -= 8;
        *((uintptr_t*) new_stackptr) = process->sighandle_trampoline;

        // Заполняем возврат
        old_frame->rdi = signum;
        old_frame->rip = sigact->handler;

        // Устанавливаем стек
        old_frame->rsp = new_stackptr;
        old_frame->rbp = new_stackptr;

        return;
    }
}

void arch_exit_process_from_handler(int exit_code, int caller, void* frame)
{
    struct thread* thr = cpu_get_current_thread();
    if (caller == CALLER_SYSCALL)
    {
        exit_process(exit_code);
    } 
    else if (caller == CALLER_SCHEDULER)
    {
        thread_frame_t* old_frame = thr->context; 

        // Заполняем возврат
        old_frame->rdi = exit_code;
        old_frame->rip = exit_process;

        // Устанавливаем стек
        old_frame->rsp = thr->kernel_stack_ptr + 4096;
        old_frame->rbp = thr->kernel_stack_ptr + 4096;

        old_frame->ss = GDT_BASE_KERNEL_DATA_SEG;
        old_frame->cs = GDT_BASE_KERNEL_CODE_SEG;

        return;
    } 
    else if (caller == CALLER_INTERRUPT)
    {
        interrupt_frame_t* old_frame = frame; 

        // Заполняем возврат
        old_frame->rdi = exit_code;
        old_frame->rip = exit_process;

        // Устанавливаем стек
        old_frame->rsp = thr->kernel_stack_ptr + 4096;
        old_frame->rbp = thr->kernel_stack_ptr + 4096;

        old_frame->ss = GDT_BASE_KERNEL_DATA_SEG;
        old_frame->cs = GDT_BASE_KERNEL_CODE_SEG;

        return;
    }
}

void arch_sigreturn()
{
    struct thread* thr = cpu_get_current_thread();

    thread_frame_t* old_frame = this_core->user_stack;

    disable_interrupts();
    scheduler_exit(old_frame);
}