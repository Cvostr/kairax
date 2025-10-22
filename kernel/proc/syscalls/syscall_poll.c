#include "proc/syscalls.h"
#include "cpu/cpu_local.h"
#include "kairax/errors.h"
#include "proc/thread_scheduler.h"
#include "proc/timer.h"
#include "kairax/string.h"

int sys_poll(struct pollfd *ufds, nfds_t nfds, int timeout)
{
    struct thread *thread = cpu_get_current_thread();
    struct process *process = thread->process;
    int rc = -1;
    int catched;
    int fd;
    short revents;

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
                rc = -ERROR_BAD_FD;
                goto exit;
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
            if (scheduler_sleep1() == 1)
            {
                rc = -EINTR;
                goto exit;
            }
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
            // TODO: сделать
        }
 
    }

exit:
    poll_unwait(&pctl);
    return rc;
}