#include "proc/syscalls.h"
#include "cpu/cpu_local.h"
#include "kairax/errors.h"
#include "proc/thread_scheduler.h"
#include "proc/timer.h"
#include "kairax/string.h"
#include "mem/kheap.h"

int sys_poll(struct pollfd *ufds, nfds_t nfds, int timeout)
{
    struct thread *thread = cpu_get_current_thread();
    struct process *process = thread->process;
    int rc = -1;
    int catched;
    int fd;
    short revents;
    int wake_reason;
    struct event_timer* timer = NULL;

    struct poll_ctl pctl;
    memset(&pctl, 0, sizeof(struct poll_ctl));

    VALIDATE_USER_POINTER_PROTECTION(process, ufds, sizeof(struct pollfd) * nfds, PAGE_PROTECTION_WRITE_ENABLE)

    while (1)
    {
        catched = 0;
        for (nfds_t i = 0; i < nfds; i ++)
        {
            struct pollfd *pfd = &ufds[i];
            fd = pfd->fd;

            if (fd < 0)
            {
                // При отрицательных дескрипторах зануляем revents и пропускаем
                pfd->revents = 0;
                continue;
            }

            struct file* file = process_get_file_ex(process, fd, TRUE);
            if (file == NULL)
            {
                // Файл отсутствует - POLLNVAL
                pfd->revents = POLLNVAL;
                catched ++;
                continue;
            }

            if (file->ops->poll)
            {
                // вызываем poll для файла
                revents = file->ops->poll(file, &pctl);
                // Маскируем ненужные события
                revents = pfd->events & revents;
                if (revents != 0)
                {
                    pfd->revents = revents;
                    catched ++;
                }
            }
            else
            {
                rc = -ERROR_BAD_FD;
                file_close(file);
                goto exit;
            }

            file_close(file);
        }

        if (catched > 0)
        {
            rc = catched;
            goto exit;
        }

        if (timeout < 0)
        {
            // Отрицательный таймаут - бесконечный сон
            wake_reason = scheduler_sleep1();
        }
        else if (timeout == 0)
        {
            // Нулевой таймаут - моментальный выход
            rc = 0;
            goto exit;
        }
        else
        {
            // Спим с таймаутом
            struct timespec ts;
            ts.tv_sec = timeout / 1000;
            ts.tv_nsec = (timeout % 1000) * 1000000;
            // Объект таймера
            timer = register_event_timer(ts);
            if (timer == NULL) {
                rc = -ENOMEM;
                goto exit;
            }
            
            // Засыпаем на таймере
            wake_reason = sleep_on_timer(timer);

            // Удаляем таймер из списка и освобождаем память
            unregister_event_timer(timer);
            kfree(timer);
        }

        // анализируем причину пробуждения
        switch (wake_reason)
        {
            case WAKE_SIGNAL:
                rc = -EINTR;
                goto exit;
            case WAKE_TIMER:
                rc = 0;
                goto exit;
            default:
                continue;
        }
    }

exit:
    poll_unwait(&pctl);
    return rc;
}