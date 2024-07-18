#include "proc/syscalls.h"
#include "cpu/cpu_local_x64.h"
#include "proc/process.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "string.h"

// X86
#include "proc/x64_context.h"
//

pid_t sys_fork() 
{
    size_t i;
    struct process* process = cpu_get_current_thread()->process;

    // Создать новый процесс
    struct process* new_process = create_new_process(process);
    // Добавить в список и назначить pid
    process_add_to_list(new_process);

    // Копировать имя
    strncpy(new_process->name, process->name, PROCESS_NAME_MAX_LEN);

    // Назначение рабочей директории
    if (process->pwd != NULL)
    {
        new_process->pwd = process->pwd;
        dentry_open(new_process->pwd);
    }

    // Процесс наследует блокированные сигналы
    new_process->blocked_signals = process->blocked_signals;

    // Процесс наследует ID пользователя
    new_process->uid = process->uid;
    new_process->euid = process->euid;

    new_process->gid = process->gid;
    new_process->egid = process->egid;

    // Скопировать флаги CLOEXEC
    memcpy(new_process->close_on_exec, process->close_on_exec, sizeof(process->close_on_exec));

    // Добавить файловые дескрипторы
    acquire_spinlock(&process->fd_lock);
    for (i = 0; i < MAX_DESCRIPTORS; i ++) 
    {
        struct file* fl = process->fds[i];
        if (fl != NULL) {
            process_add_file_at(new_process, fl, i);
        }
    }
    release_spinlock(&process->fd_lock);

    // Копирование регионов памяти
    acquire_spinlock(&process->mmap_lock);
    for (i = 0; i < list_size(process->mmap_ranges); i ++) 
    {
        struct mmap_range* range = list_get(process->mmap_ranges, i);
        
        struct mmap_range* new_range = kmalloc(sizeof(struct mmap_range));
        memcpy(new_range, range, sizeof(struct mmap_range));

        int is_shared = (new_range->flags & MAP_SHARED) == MAP_SHARED;

        page_table_mmap_fork(
            process->vmemory_table->arch_table, 
            new_process->vmemory_table->arch_table, new_range, !is_shared);

        process_add_mmap_region(new_process, new_range);
    }
    release_spinlock(&process->mmap_lock);

    // Перенос BRK, чтобы не замапить 0 адрес
    new_process->brk = process->brk;

    // TLS
    new_process->tls_size = process->tls_size;
    new_process->tls = kmalloc(new_process->tls_size);
    memcpy(new_process->tls, process->tls, new_process->tls_size);

    // Копирование контекста
    // Платформозависимо!!!
    syscall_frame_t* syscall_frame = this_core->kernel_stack;
    syscall_frame = syscall_frame - 1;

    // Создание главного потока без стека, т.к стек установим дальше
    struct thread* thr = create_thread(new_process, syscall_frame->rcx, 0, -1, NULL);
    thread_frame_t* ctx = thr->context;

    ctx->rax = 0;   
    ctx->rbx = syscall_frame->rbx;
    ctx->rdx = syscall_frame->rdx;
    ctx->rdi = syscall_frame->rdi;
    ctx->rsi = syscall_frame->rsi;
    ctx->rsp = get_user_stack_ptr();
    ctx->rbp = syscall_frame->rbp;
    ctx->r8 = syscall_frame->r8;
    ctx->r9 = syscall_frame->r9;
    ctx->r10 = syscall_frame->r10;
    ctx->rflags = syscall_frame->r11;
    ctx->r12 = syscall_frame->r12;
    ctx->r13 = syscall_frame->r13;
    ctx->r14 = syscall_frame->r14;
    ctx->r15 = syscall_frame->r15;

	// Добавление потока в планировщик
    scheduler_add_thread(thr);
    process_add_to_list(thr);

    return new_process->pid;
}

pid_t sys_vfork() 
{
    return sys_fork();
}