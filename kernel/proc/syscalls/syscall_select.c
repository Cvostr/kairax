#include "proc/syscalls.h"
#include "cpu/cpu_local.h"
#include "kairax/string.h"
#include "kairax/select.h"
#include "proc/timer.h"

#define FD_ZERO(set)        (memset ((void*) (set), 0, sizeof(fd_set)))
#define FD_SET(d, set)	    ((set)->fds_bits[__FDSET_BYTE(d)] |= __MAKE_FDMASK(d))
#define FD_CLR(d, set)	    ((set)->fds_bits[__FDSET_BYTE(d)] &= ~__MAKE_FDMASK(d))
#define FD_ISSET(d, set)	(((set)->fds_bits[__FDSET_BYTE(d)] & __MAKE_FDMASK(d)) != 0)

int sys_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    struct thread *thread = cpu_get_current_thread();
    struct process *process = thread->process;
    struct file* file;
    int rc;
    short revents;
    int catched;
    int wake_reason;
    struct event_timer* timer = NULL;
    struct timespec ts;

    struct poll_ctl pctl;
    memset(&pctl, 0, sizeof(struct poll_ctl));

    if (n < 0)
    {
        return -EINVAL;
    }

    if (readfds)
        VALIDATE_USER_POINTER(process, readfds, sizeof(fd_set))
    if (writefds)
        VALIDATE_USER_POINTER(process, writefds, sizeof(fd_set))
    if (exceptfds)
        VALIDATE_USER_POINTER(process, exceptfds, sizeof(fd_set))
    if (timeout)
        VALIDATE_USER_POINTER(process, timeout, sizeof(struct timeval))

    fd_set _readfds, _writefds;
    FD_ZERO(&_readfds);
    FD_ZERO(&_writefds);

    while (1)
    {
        catched = 0;
        for (int i = 0; i < n; i ++)
        {
            short reqdpollmask = 0;
            if ((readfds != NULL) && FD_ISSET(i, readfds))
                reqdpollmask |= POLLIN;
            if ((writefds != NULL) && FD_ISSET(i, writefds))
                reqdpollmask |= POLLOUT;
            // TODO: exceptfds

            if (reqdpollmask != 0)
            {
                file = process_get_file_ex(process, i, TRUE);
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
                    revents = reqdpollmask & revents;
                    // Смотрим события
                    if ((revents & POLLIN) != 0)
                    {
                        catched ++;
                        FD_SET(i, &_readfds);
                    }

                    if ((revents & POLLOUT) != 0)
                    {
                        catched ++;
                        FD_SET(i, &_writefds);
                    }
                    // TODO: exceptfds
                }
                else
                {
                    rc = -ERROR_BAD_FD;
                    file_close(file);
                    goto exit;
                }


                file_close(file);
            }
        }

        if (catched > 0)
        {
            rc = catched;
            goto exit;
        }

        if (timeout == NULL)
        {
            // timeout = NULL - бесконечный сон
            wake_reason = scheduler_sleep1(); 
        }
        else
        {
            if (timeout->tv_sec == 0 && timeout->tv_usec == 0)
            {
                // Если оба значения 0 - значит сразу выходим
                rc = 0;
                goto exit;
            }

            ts.tv_sec = timeout->tv_sec;
            ts.tv_nsec = timeout->tv_usec * 1000;
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
    if (readfds != NULL)
        memcpy(readfds, &_readfds, sizeof(fd_set));
    if (writefds)
        memcpy(writefds, &_writefds, sizeof(fd_set));
    return rc;
}