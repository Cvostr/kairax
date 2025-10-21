#include "proc/syscalls.h"
#include "cpu/cpu_local.h"
#include "kairax/string.h"
#include "kairax/select.h"

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

    struct poll_ctl pctl;
    memset(&pctl, 0, sizeof(struct poll_ctl));
    
    if (readfds)
        VALIDATE_USER_POINTER(process, readfds, sizeof(fd_set))
    if (writefds)
        VALIDATE_USER_POINTER(process, writefds, sizeof(fd_set))
    if (exceptfds)
        VALIDATE_USER_POINTER(process, exceptfds, sizeof(fd_set))
    if (timeout)
        VALIDATE_USER_POINTER(process, timeout, sizeof(struct timeval))

    while (1)
    {
        catched = 0;
        for (int i = 0; i < FD_SETSIZE; i ++)
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
                        FD_SET(i, readfds);
                    } else
                        FD_CLR(i, readfds);
                    if ((revents & POLLOUT) != 0)
                    {
                        catched ++;
                        FD_SET(i, writefds);
                    } else
                        FD_CLR(i, writefds);
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

        if (scheduler_sleep1() == 1)
        {
            rc = -EINTR;
            goto exit;
        }
    }

exit:
    poll_unwait(&pctl);
    return rc;
}