#include "poll.h"
#include "mem/kheap.h"
#include "fs/vfs/file.h"
#include "cpu/cpu_local.h"

int poll_intrusive_add(struct poll_queued** head, struct poll_queued** tail, struct poll_queued* pq)
{
    if (pq->prev != NULL || pq->next != NULL) {
        // Если поток уже в каком либо списке - выходим
        return -1;
    }

    if (!(*head)) {
        // Первого элемента не существует
        *head = pq;
        pq->prev = NULL;
    } else {
        (*tail)->next = pq;
        pq->prev = (*tail);
    }

    // Добавляем в конец - следующего нет
    pq->next = NULL;

    *tail = pq;

    return 0;
}

int poll_intrusive_remove(struct poll_queued** head, struct poll_queued** tail, struct poll_queued* pq)
{
    struct futex* prev = pq->prev;
    struct futex* next = pq->next;

    if (prev) {
        pq->next = next;
    }

    if (next) {
        pq->prev = prev;
    }

    if (*head == pq) {
        *head = next;
        if (*head) (*head)->prev = NULL;
    }

    if (*tail == pq) {
        *tail = prev;
        if (*tail) (*tail)->next = NULL;
    }

    pq->next = NULL;
    pq->prev = NULL;

    return 0;
}

void poll_wait(struct poll_ctl* ctl, struct file* file, struct poll_wait_queue* q)
{
    struct thread *thread = cpu_get_current_thread();
    struct poll_queued* pqueued;

    // Добавить в очередь для файла
    acquire_spinlock(&q->lock);
    if (list_get_node(&q->threads, thread) == NULL)
    {
        list_add(&q->threads, thread);
    }
    release_spinlock(&q->lock);

    // Проверить, есть ли очередь в списке
    pqueued = ctl->head;
    while (pqueued != NULL)
    {
        if (pqueued->file == file && pqueued->queue == q)
        {
            return;
        }

        pqueued = pqueued->next;
    }

    // Добавить в список для удаления после выхода
    pqueued = kmalloc(sizeof(struct poll_queued));
    file_acquire(file);
    pqueued->file = file;
    pqueued->queue = q;
    pqueued->next = NULL;
    pqueued->prev = NULL;
    poll_intrusive_add(&ctl->head, &ctl->tail, pqueued);
}

void poll_wakeall(struct poll_wait_queue* q)
{
    struct thread* thr = NULL;

    acquire_spinlock(&q->lock);
    while ((thr = list_dequeue(&q->threads)) != NULL)
    {
        scheduler_wakeup1(thr);
    }
    release_spinlock(&q->lock);
}

void poll_unwait(struct poll_ctl *pctl)
{
    struct thread *thread = cpu_get_current_thread();
    struct poll_queued* queued;
    while ((queued = pctl->head) != NULL)
    {
        //printk("poll_unwait\n");
        struct poll_wait_queue* queue = queued->queue;
        acquire_spinlock(&queue->lock);
        list_remove(&queue->threads, thread);
        release_spinlock(&queue->lock);

        file_close(queued->file);

        poll_intrusive_remove(&pctl->head, &pctl->tail, queued);
        kfree(queued);
    }
}