#include "types.h"
#include "stdio.h"
#include "thread_scheduler.h"

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

extern void cpu_yield();

void syscall_handle(syscall_frame_t* frame) {
    char* mem = (char*)frame->rdi;
    thread_t* current_thread = scheduler_get_current_thread();
    process_t* current_process = current_thread->process;
    size_t buffer_length = 0;
    
    switch (frame->rax) {
        case 1:
            printf("%s", mem);
            break;

        case 0x0: // Чтение файла
            frame->rax = process_read_file(current_process, frame->rdi, (char*)frame->rsi, frame->rdx);
            break;

        case 0x2: // Открытие файла
            frame->rax = process_open_file(current_process, 
               (char*)frame->rdi,
               frame->rsi,
               frame->rdx);
            break;

        case 0x3: // Закрытие файла
            frame->rax = process_close_file(current_process, 
               frame->rdi);
            break;

        case 0x5: // Закрытие файла
            frame->rax = process_stat(current_process, 
               frame->rdi, (struct stat*)frame->rsi);
            break;

        case 0x18:
            current_thread->state = THREAD_UNINTERRUPTIBLE; // Ожидающий системный вызов
            cpu_yield();
            current_thread->state = THREAD_RUNNING;
            break;

        case 0x23:
            current_thread->state = THREAD_UNINTERRUPTIBLE; // Ожидающий системный вызов
            for (uint64_t i = 0; i < frame->rdi; i ++) {
                cpu_yield();
            }
            current_thread->state = THREAD_RUNNING;
            break;

        case 0x27:  //Получение PID процесса
            frame->rax = current_process->pid;
            break;

        case 0xBA:  // Получение ID потока
            frame->rax = current_thread->thread_id;
            break;

        case 0x4F:  // Получение директории
            buffer_length = frame->rsi;
            frame->rax = process_get_working_dir(current_process, mem, buffer_length);
            break;

        case 0x50:  // Установка директории
            frame->rax = process_set_working_dir(current_process, mem);
            break;

        case 0x59:
            frame->rax = process_readdir(current_process, frame->rdi, (struct dirent*)frame->rsi);
            break;

        case 0x3C:  //Завершение процесса
            scheduler_remove_process_threads(current_process);
            cpu_yield();
            break;
    }
}