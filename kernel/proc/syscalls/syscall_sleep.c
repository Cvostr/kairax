#include "proc/syscalls.h"
#include "proc/timer.h"
#include "mem/kheap.h"
#include "cpu/cpu_local.h"

int sys_sleep(time_t sec, long int nsec)
{
    int rc = 0;

    if (sec == 0 && nsec == 0) {
        goto exit;
    }

    if (sec < 0 || nsec < 0) {
        rc = -ERROR_INVALID_VALUE;
        goto exit;
    }

    struct thread* thread = cpu_get_current_thread();
    struct timespec duration = {.tv_sec = sec, .tv_nsec = nsec};
    struct event_timer* timer = create_and_register_event_timer(duration);
    if (timer == NULL) {
        return -ENOMEM;
    }
    
    if (sleep_on_timer(timer) == WAKE_SIGNAL)
    {
        rc = -EINTR;
    }

    unregister_event_timer(timer);
    kfree(timer);

exit:
    return rc;
}

int sys_pause()
{
    scheduler_sleep1();
    return -EINTR;
}

unsigned int sys_alarm(unsigned int seconds)
{
    unsigned int remaining;
    struct event_timer  *alarm_timer;
    struct thread* thread = cpu_get_current_thread();

    alarm_timer = &thread->alarm_timer;

    // Для бОльшей стабильности всегда вытаскиваем таймер из списка
    unregister_event_timer(&alarm_timer);

    // сохранить оставшееся время
    remaining = alarm_timer->when.tv_sec;

    // Если 0, то снимаем таймер, который уже был настроен
    if (seconds == 0)
    {
        return remaining;
    }

    // Обновляем/устанавливаем оставшееся время
    alarm_timer->when.tv_sec = seconds;
    alarm_timer->when.tv_nsec = 0;

    alarm_timer->wait_head = thread;
    thread->timer = alarm_timer;
    alarm_timer->sig = TRUE;

    register_event_timer(&thread->alarm_timer);

    return remaining;
}