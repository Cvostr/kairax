#include "bootshell.h"
#include "dev/keyboard/int_keyboard.h"
#include "bootshell_cmdproc.h"
#include "stdio.h"
#include "drivers/video/video.h"
#include "proc/syscalls.h"
#include "string.h"
#include "proc/thread.h"
#include "proc/thread_scheduler.h"

struct process* bootshell_spawn_new_process(struct process* parent)
{
    struct process* proc = create_new_process(parent);
	process_set_name(proc, "bootshell");
	// Добавить в список и назначить pid
    process_add_to_list(proc);

    struct thread* thr = create_kthread(proc, bootshell, NULL);
    thread_set_name(thr, "bootshell main thread");
	process_add_to_list((struct process*) thr);
	scheduler_add_thread(thr);

    return proc;
}

void bootshell_print_sign()
{
    char curdir[512];
    memset(curdir, 0, sizeof(curdir));
    sys_get_working_dir(curdir, 512);
    printf_stdout("\033[1;92mKAIRAX\033[0m:\033[1;94m%s\033[0m# ", curdir);
}

void bootshell()
{
    bootshell_print_sign();

    while(1) {

        char command[256];
        kfgets(command, 256);

        bootshell_process_cmd(command);
        bootshell_print_sign();

    }
}