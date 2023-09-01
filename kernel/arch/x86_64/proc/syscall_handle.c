#include "types.h"
#include "stdio.h"
#include "proc/thread_scheduler.h"
#include "cpu/cpu_local_x64.h"
#include "proc/syscalls.h"

typedef struct PACKED 
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rax;

    // Задаются вызовом SYSCALL
    uint64_t rcx;
    uint64_t r11;
} syscall_frame_t;

void syscall_handle(syscall_frame_t* frame) {
    char* mem = (char*)frame->rdi;
    struct thread* current_thread = cpu_get_current_thread();
    struct process* current_process = current_thread->process;
    
    switch (frame->rax) {
        
        case 0x0: // Чтение файла
            frame->rax = sys_read_file(frame->rdi, (char*)frame->rsi, frame->rdx);
            break;

        case 0x1:
            frame->rax = sys_write_file(frame->rdi, (const char*)frame->rsi, frame->rdx);
            break;

        case 0x2: // Открытие файла
            frame->rax = sys_open_file( 
               (int)frame->rdi,
               (char*)frame->rsi,
               frame->rdx,
               frame->r8);
            break;

        case 0x3: // Закрытие файла
            frame->rax = sys_close_file((int)frame->rdi);
            break;

        case 0x5: // stat
            frame->rax = sys_stat( 
               frame->rdi, frame->rsi, (struct stat*)frame->rdx, frame->r8);
            break;

        case 0x8: // перемещение каретки файла
            frame->rax = sys_file_seek( 
               frame->rdi, frame->rsi, frame->rdx);
            break;

        case 0x10: // ioctl
            frame->rax = sys_ioctl( 
               frame->rdi, frame->rsi, frame->rdx);
            break;

        case 0x18:
            current_thread->state = THREAD_UNINTERRUPTIBLE; // Ожидающий системный вызов
            scheduler_yield();
            current_thread->state = THREAD_RUNNING;
            break;

        case 0x23:
            sys_thread_sleep(frame->rdi);
            break;

        case 0x27:  //Получение PID процесса
            frame->rax = sys_get_process_id();
            break;

        case 0xBA:  // Получение ID потока
            frame->rax = sys_get_thread_id();
            break;

        case 0x4F:  // Получение директории
            frame->rax = sys_get_working_dir(mem, frame->rsi);
            break;

        case 0x50:  // Установка директории
            frame->rax = sys_set_working_dir(mem);
            break;

        case 0x53:
            frame->rax = sys_mkdir(frame->rdi, frame->rsi, frame->rdx);
            break;

        case 0x59:
            frame->rax = sys_readdir((int)frame->rdi, (struct dirent*)frame->rsi);
            break;

        case 0x3C:  //Завершение процесса
            sys_exit_process(frame->rdi);
            break;

        case 0xFF10:  // Создание потока
            frame->rax = process_create_thread(  current_process,
                                    (void*)frame->rdi,
                                    (void*)frame->rsi,
                                    (pid_t*)frame->rdx,
                                    frame->r8);
            scheduler_yield();
            break;
    }
}