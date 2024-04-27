#include "kterm.h"
#include "proc/thread.h"
#include "misc/bootshell/bootshell.h"
#include "proc/syscalls.h"
#include "stdio.h"
#include "keycodes.h"
#include "dev/keyboard/int_keyboard.h"
#include "mem/kheap.h"
#include "string.h"

struct process *kterm_process = NULL;

#define KTERM_SESSIONS_MAX 12
struct terminal_session* kterm_sessions[KTERM_SESSIONS_MAX];

struct vgaconsole* current_console = NULL;
struct terminal_session* current_session = NULL;

void kterm_process_start()
{
    // Первоначальный процесс kterm
	kterm_process = create_new_process(NULL);
	// Добавить в список и назначить pid
    process_add_to_list(kterm_process);

	struct thread* thr = create_kthread(kterm_process, kterm_main, NULL);
	scheduler_add_thread(thr);
	process_add_to_list(thr);
}

char keyboard_get_key_ascii(char keycode);

struct terminal_session* new_kterm_session(int create_console) 
{
	struct terminal_session* session = kmalloc(sizeof(struct terminal_session));
	memset(session, 0, sizeof(struct terminal_session));
	struct kterm_color foreground_color = DEFAULT_FOREGROUND_COLOR;
	session->foreground_color = foreground_color;

	sys_create_pty(&session->master, &session->slave);

	struct file *slave_file = process_get_file(kterm_process, session->slave);

	// Процесс shell
	session->proc = create_new_process(kterm_process);
	// Добавить в список и назначить pid
    process_add_to_list(session->proc);
	// Переопределить каналы
	process_add_file_at(session->proc, slave_file, 0);
	process_add_file_at(session->proc, slave_file, 1);
	process_add_file_at(session->proc, slave_file, 2);

	struct thread* thr = create_kthread(session->proc, bootshell, NULL);
	scheduler_add_thread(thr);
	process_add_to_list(thr);

	if (create_console == TRUE) {
		session->console = console_init();
	}

	return session;
}

void kterm_change_to(int index) 
{
	if (index >= KTERM_SESSIONS_MAX)
		return;

	struct terminal_session* session = kterm_sessions[index];
	if (session == NULL) {
		session = new_kterm_session(TRUE);
		kterm_sessions[index] = session;
	}

	if (current_session == session)
		return;

	current_session = session;
	current_console = session->console;

	console_redraw(current_console);
}

void kterm_main()
{
	struct terminal_session* session = new_kterm_session(FALSE);
	current_session = session;
	kterm_sessions[0] = session;
	session->console = current_console;

	while (1) {

		kterm_session_process(current_session);

		short keycode_ext = keyboard_get_key();
		if (keycode_ext > 0) {

			char keycode = (keycode_ext) & 0xFF;
			int state = (keycode_ext >> 8) & 0xFF;

			if (keycode >= KRXK_F1 && keycode <= KRXK_F12) {
				int index = keycode - KRXK_F1; 
				kterm_change_to(index);
			}

			switch (keycode) {
				case KRXK_LCTRL:
				case KRXK_RCTRL:
					current_session->ctrl_hold = (state == 0);
					break;

				default:
					if (state == 0) {
						char symbol = keyboard_get_key_ascii(keycode);

						if (current_session->ctrl_hold) {
							char chr = 0;
							switch (symbol) {
								case 'c':
									chr = 3;
									sys_write_file(current_session->master, &chr, 1);
									break;
								case 'h':
									chr = '\b';
									sys_write_file(current_session->master, &chr, 1);
									break;
							}
							break;
						}

						if (symbol > 0)
							sys_write_file(current_session->master, &symbol, 1);
					}
				break;
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
	case KRXK_SEMICOLON:
		return ';';
      break;
    default:
        //if (keycode != 0)
        //printf("%i ", keycode);
	}
	return 0;
}