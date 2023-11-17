#include "kterm.h"
#include "vgaterm.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "misc/bootshell/bootshell.h"
#include "proc/syscalls.h"
#include "stdio.h"
#include "keycodes.h"
#include "dev/keyboard/int_keyboard.h"

struct process *kterm_process = NULL;
int master, slave;

void kterm_process_start()
{
    // Первоначальный процесс kterm
	kterm_process = create_new_process(NULL);
	// Добавить в список и назначить pid
    process_add_to_list(kterm_process);

	struct thread* thr = create_kthread(kterm_process, kterm_main);
	scheduler_add_thread(thr);
}

char keyboard_get_key_ascii(char keycode);

void kterm_main()
{
	console_init();
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

			char c = buff[i];
			switch (c) {
				case '\b':
					console_backspace(1);
					break;
				case '\r':
					console_cr();
					break;
				case '\n':
					console_lf();
					break;
				default:
					console_print_char(c);
			}
		}

		short keycode_ext = keyboard_get_key();
		if (keycode_ext > 0) {

			char keycode = (keycode_ext) & 0xFF;
			int state = (keycode_ext >> 8) & 0xFF;

			if (state == 0) {
				char symbol = keyboard_get_key_ascii(keycode);
				sys_write_file(master, &symbol, 1);
			}
		}
	}
}


char keyboard_get_key_ascii(char keycode)
{
	switch(keycode) {
    case KRXK_Q:
        return 'q';
    case KRXK_W:
        return 'w';
    case KRXK_E:
      	return 'e';
    case KRXK_R:
      	return 'r';
    case KRXK_T:
      	return 't';
    case KRXK_Y:
      	return 'y';
    case KRXK_U:
      	return 'u';
    case KRXK_I:
      	return 'i';
    case KRXK_O:
      	return 'o';
    case KRXK_P:
      	return 'p';
    case KRXK_A:
      	return 'a';
    case KRXK_S:
      	return 's';
    case KRXK_D:
      	return 'd';
    case KRXK_F:
      	return 'f';
    case KRXK_G:
      	return 'g';
    case KRXK_H:
      	return 'h';
    case KRXK_J:
      	return 'j';
    case KRXK_K:
      	return 'k';
    case KRXK_L:
      	return 'l';
    case KRXK_Z:
      	return 'z';
    case KRXK_X:
      	return 'x';
    case KRXK_C:
      	return 'c';
    case KRXK_V:
      	return 'v';
    case KRXK_B:
      	return 'b';
    case KRXK_N:
      	return 'n';
    case KRXK_M:
      	return 'm';
    case KRXK_1:
      	return '1';
    case KRXK_2:
      	return '2';
    case KRXK_3:
      	return '3';
    case KRXK_4:
      	return '4';
    case KRXK_5:
      	return '5';
    case KRXK_6:
      	return '6';
    case KRXK_7:
      	return '7';
    case KRXK_8:
      	return '8';
    case KRXK_9:
        return '9';
    case KRXK_0:
        return '0';
    case KRXK_MINUS:
        return '-';
    case KRXK_ENTER:
        return '\n';
    case KRXK_BKSP:
        return '\b';
    case KRXK_SPACE:
      	return ' ';
    case KRXK_SLASH:
      return '/';
    case KRXK_DOT:
      return '.';
      break;
    default:
        //if (keycode != 0)
        //printf("%i ", keycode);
	}
	return 0;
}