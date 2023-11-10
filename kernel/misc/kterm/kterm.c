#include "kterm.h"
#include "vgaterm.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "misc/bootshell/bootshell.h"
#include "proc/syscalls.h"
#include "stdio.h"

struct process *kterm_process = NULL;

void kterm_process_start()
{
    // Первоначальный процесс kterm
	kterm_process = create_new_process(NULL);
	// Добавить в список и назначить pid
    process_add_to_list(kterm_process);

	struct thread* thr = create_kthread(kterm_process, kterm_main);
	scheduler_add_thread(thr);
}

void kterm_main()
{
    int master, slave;
    sys_create_pty(&master, &slave);
	struct file *slave_file = process_get_file(kterm_process, slave);

	struct process* proc = create_new_process(NULL);
	// Добавить в список и назначить pid
    process_add_to_list(proc);
	// Переопределить каналы
	process_add_file_at(proc, slave_file, 0);
	process_add_file_at(proc, slave_file, 1);
	process_add_file_at(proc, slave_file, 2);

	struct thread* thr = create_kthread(proc, bootshell);
	scheduler_add_thread(thr);

	while (1) {
		char buff[128];
		ssize_t n = sys_read_file(master, buff, 128);

		for (int i = 0; i < n; i ++) {
			console_print_char(buff[i]);
		}

		/*char keycode = keyboard_get_key();
		if (keycode > 0) {
        char symbol = keyboard_get_key_ascii(keycode);
		sys_write_file(0, &symbol, 1);
		}*/
	}
}