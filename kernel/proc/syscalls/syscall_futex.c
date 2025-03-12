#include "proc/syscalls.h"
#include "cpu/cpu_local_x64.h"
#include "proc/thread_scheduler.h"
#include "mem/kheap.h"
#include "string.h"

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

struct futex {

    uintptr_t futex;
    struct blocker blocker;

    struct futex_holder* prev;
    struct futex_holder* next;
};

struct futex* new_futex() 
{
    struct futex* futex = kmalloc(sizeof(struct futex));
    memset(futex, 0, sizeof(struct futex));
    return futex;
}

int futex_intrusive_add(struct futex** head, struct futex** tail, struct futex* futex)
{
    if (futex->prev != NULL || futex->next != NULL) {
        // Если поток уже в каком либо списке - выходим
        return -1;
    }

    if (!(*head)) {
        // Первого элемента не существует
        *head = futex;
        futex->prev = NULL;
    } else {
        (*tail)->next = futex;
        futex->prev = (*tail);
    }

    // Добавляем в конец - следующего нет
    futex->next = NULL;

    *tail = futex;

    return 0;
}

int futex_intrusive_remove(struct futex** head, struct futex** tail, struct futex* futex)
{
    struct futex* prev = futex->prev;
    struct futex* next = futex->next;

    if (prev) {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }

    if (*head == futex) {
        *head = next;
        if (*head) (*head)->prev = NULL;
    }

    if (*tail == futex) {
        *tail = prev;
        if (*tail) (*tail)->next = NULL;
    }

    futex->next = NULL;
    futex->prev = NULL;

    return 0;
}

struct futex* process_get_futex(struct process* process, uintptr_t futex_addr)
{
    struct futex* result = NULL;
    acquire_spinlock(&process->futex_list_lock);

    struct futex* current = process->futex_list_head;

    while (current != NULL) 
    {
        if (current->futex == futex_addr) {
            result = current;
            goto exit;
        }

        current = current->next;
    }

exit:
    release_spinlock(&process->futex_list_lock);
    return result;
}

int sys_futex(void* futex, int op, int val, const struct timespec *timeout)
{
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    struct futex* futx = NULL;

    VALIDATE_USER_POINTER(process, futex, sizeof(int))

    if (op == FUTEX_WAIT)
    {
        if (*((int*)futex) != val) {
            return -EAGAIN;
        }

        futx = process_get_futex(process, futex);

        if (futx == NULL) 
        {
            futx = new_futex();
            futx->futex = futex;
            acquire_spinlock(&process->futex_list_lock);
            futex_intrusive_add(&process->futex_list_head, &process->futex_list_tail, futx);
            release_spinlock(&process->futex_list_lock);
        }

        scheduler_sleep_on(&futx->blocker);
       
    } else if (op == FUTEX_WAKE) {
        //printk(" WOKE UP  %i  ", futex_physaddr);

        futx = process_get_futex(process, futex);
        if (futx == NULL) {
            return 0; // ???
        }

        int r = scheduler_wakeup_intrusive(&futx->blocker.head, &futx->blocker.tail, &futx->blocker.lock, val);

        if (futx->blocker.head == NULL && futx->blocker.tail == NULL)
        {
            // Разбудили всех - можно убивать объект futex
            acquire_spinlock(&process->futex_list_lock);
            futex_intrusive_remove(&process->futex_list_head, &process->futex_list_tail, futx);
            release_spinlock(&process->futex_list_lock);
            kfree(futx);
        }

        return r;
    }

    return 0;
}