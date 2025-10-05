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
#include "kairax/intctl.h"
#include "kstdlib.h"
#include "cpu/cpu_x64.h"
#include "interrupts/apic.h"
#include "memory/kernel_vmm.h"

extern void scheduler_yield_entry();
extern void scheduler_exit(thread_frame_t* ctx);
int scheduler_handler(thread_frame_t* frame);
int scheduler_enabled = 0;

extern struct cpu_local_x64** cpus;
extern size_t cpus_num; 

#define DEFAULT_TIMESLICE 3
#define BALANCE_PERIOD      100

uint32_t scheduler_get_less_loaded()
{
    uint32_t min = UINT32_MAX;
    uint32_t pos = 0;

    for (uint32_t i = 0; i < cpus_num; i ++)
    {
        struct sched_wq* wq = cpus[i]->wq;
        if (wq->size < min) 
        {
            min = wq->size;
            pos = i;
        }
    }

    return pos;
}

void cpu_reshedule_handler()
{
    struct sched_wq* wq = cpu_get_wq();
    struct thread* mthr = this_core->migration_thread;

    if (mthr != NULL) {
        wq_add_thread(wq, mthr);
        this_core->migration_thread = NULL;
    }

    this_core->migration_lock = 0;
}

void cpu_put_thread(uint32_t icpu, struct thread* thread)
{
    struct cpu_local_x64* cpu = cpus[icpu];

    acquire_spinlock(&cpu->migration_lock);
    
    cpu->migration_thread = thread;
    lapic_send_ipi(cpu->lapic_id, IPI_DST_BY_ID, IPI_TYPE_FIXED, INTERRUPT_VEC_RES);
}

void scheduler_yield(int save_context)
{
    // Выключение прерываний
    disable_interrupts();

    struct thread* thr = cpu_get_current_thread();
    if (thr != NULL) {
        thr->timeslice = 0;
    }

    if (save_context) {
        scheduler_yield_entry();
    } else {
        cpu_set_current_thread(NULL);

        // Переход в планировщик
        scheduler_handler(NULL);    
    }   
}

void balance()
{
    struct sched_wq* wq = cpu_get_wq();

    if (wq->size == 0) {
        return;
    }

    wq->since_balance -= 1;

    if (wq->since_balance > 0) {
        return;
    }

    uint32_t index = scheduler_get_less_loaded();
    int id = cpu_get_id();
    if (index != id)
    { 
        struct sched_wq* wq = cpu_get_wq();
        struct thread* last = wq->tail;
        if (last->balance_forbidden == FALSE)
        {
            wq_remove_thread(wq, last);
            last->cpu = index;
            cpu_put_thread(index, last);
        }
        //printk("balanced from %i to %i\n", id, index);
    }

    wq->since_balance = BALANCE_PERIOD;
}

// frame может быть NULL
// Если это так, то мы сюда попали из убитого потока
int scheduler_handler(thread_frame_t* frame)
{
    if (!scheduler_enabled) {
        return 0;
    }

    struct thread* previous_thread = cpu_get_current_thread();

    // Сохранить состояние 
    if (previous_thread != NULL && frame) {

        // Разрешить прерывания
        frame->rflags |= 0x200;

        if ((--previous_thread->timeslice) > 0) {
            scheduler_exit(frame);
        }

        // Сохранить указатель на контекст
        previous_thread->context = frame;
        
        // Сохранить состояние FPU
        if (previous_thread->fpu_context) {
            fpu_save(previous_thread->fpu_context);
        }

        // Если процесс не блокировался - сменить состояние
        if (previous_thread->state == STATE_RUNNING)
            previous_thread->state = STATE_RUNNABLE;

        if (previous_thread->is_userspace) {
            // Сохранить указатель на вершину стека пользователя
            previous_thread->stack_ptr = get_user_stack_ptr();
        }
    }

    // Вызов простейшего балансировщика между ядрами
    // Закомментируйте, чтобы выключить многоядерность
    balance();

    // Найти следующий поток
    struct thread* new_thread = scheduler_get_next_runnable_thread();
    new_thread->state = STATE_RUNNING;
    new_thread->timeslice = DEFAULT_TIMESLICE;

    // Получить данные процесса, с которым связан поток
    struct process* process = new_thread->process;
    thread_frame_t* thread_frame = (thread_frame_t*)new_thread->context;

    // Разрешить прерывания
    thread_frame->rflags |= 0x200;

    if (new_thread->is_userspace) {
        // Обновить данные об указателях на стек
        cpu_set_kernel_stack(new_thread->kernel_stack_ptr + PAGE_SIZE);
        tss_set_rsp0((uintptr_t)new_thread->kernel_stack_ptr + PAGE_SIZE);
        set_user_stack_ptr(new_thread->stack_ptr);
    
        if (new_thread->tls != NULL) {
            // TLS, обязательно конец памяти
            cpu_set_fs_base(new_thread->tls + process->tls_size);
        }
        if (new_thread->fpu_context)
            fpu_restore(new_thread->fpu_context);
    }
    
    // Сохранить указатель на новый поток в локальной структуре ядра
    cpu_set_current_thread(new_thread);

    // Обработать сигналы, если процесс был в userspace
    if (thread_frame->cs == 0x23) {
        process_handle_signals(CALLER_SCHEDULER, thread_frame);
    }

    // Заменить таблицу виртуальной памяти процесса
    if (cpu_get_current_vm_table() != process->vmemory_table && process->vmemory_table != NULL) {  
        cpu_set_current_vm_table(process->vmemory_table);
        switch_pml4(V2P(process->vmemory_table->arch_table));
    } 
    else if (process->vmemory_table == NULL) {
        // Процесс завершается
        // Используем основную таблицу ядра
        vmm_use_kernel_vm();
        // Чтобы нормально работало переключение 
        cpu_set_current_vm_table(NULL);
    }

    scheduler_exit(thread_frame);
}

void scheduler_start()
{
    scheduler_enabled = 1;
}