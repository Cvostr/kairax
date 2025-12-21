#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "proc/process.h"
#include "cpu/cpu_local.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "string.h"
#include "proc/timer.h"

#define WNOHANG		0x00000001

pid_t sys_wait(pid_t id, int* status, int options)
{
    pid_t       result = -1;
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    acquire_spinlock(&process->wait_lock);

    struct process* child = NULL;
        
    if (id > 0) {

        // pid указан, ищем процесс
        for (;;) {

            // получим процесс по PID c увеличением счетчика ссылок
            child = process_get_child_by_id(process, id);

            if (!child) {

                child = process_get_thread_by_id(process, id);
                
                if (!child) {
                    result = -ERROR_NO_CHILD;
                    goto exit;
                }
            }            

            if (child->state == STATE_ZOMBIE) 
            {
                break;
            }
            else if ((options & WNOHANG) == WNOHANG)
            {
                free_process(child);
                result = 0;
                goto exit;
            }

            release_spinlock(&process->wait_lock);
            
            free_process(child);
            
            if (scheduler_sleep_on(&process->wait_blocker) == 1)
            {
                return -EINTR;
            }
            
            acquire_spinlock(&process->wait_lock);
        }
    } else if (id == -1) {
        // Завершить любой дочерний процесс, который зомби
        for (;;)
        {
            child = process_get_first_zombie(process);

            if (child != NULL) 
            {
                break;
            }
            else if ((options & WNOHANG) == WNOHANG)
            {
                free_process(child);
                result = 0;
                goto exit;
            }

            release_spinlock(&process->wait_lock);
            
            free_process(child);

            if (scheduler_sleep_on(&process->wait_blocker) == 1)
            {
                return -EINTR;
            }
            
            acquire_spinlock(&process->wait_lock);
        }
    } else {
        goto exit;
    }

    // Процесс завершен
    // Сохранить код вовзрата
    int code = child->code;

    // Удалить процесс из списка
    process_remove_from_list(child);

    if (child->type == OBJECT_TYPE_PROCESS) {
        // Удалить процесс из списка потомков родителя
        process_remove_child(process, child);
        // Уничтожить объект процесса
        free_process(child);
    } else if (child->type == OBJECT_TYPE_THREAD) {
        // Удалить процесс из списка потоков процесса
        process_remove_thread(process, child);
        // Уничтожить объект потока
        thread_destroy(child);
    }
        
    *status = code;
    result = id;

exit:
    release_spinlock(&process->wait_lock);
    return result;
}