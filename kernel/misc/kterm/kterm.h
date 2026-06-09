#ifndef _KTERM_H
#define _KTERM_H

#include "vgaterm.h"
#include "proc/process.h"

#define KTERM_SESSION_BUFFER_SIZE 128

#define DEFAULT_FOREGROUND_COLOR {.r = 170, .g = 170, .b = 170}
#define DEFAULT_BACKGROUND_COLOR {.r = 0, .g = 0, .b = 0}

#define ESC         0x1B    //(\033)

#define CHARTABLE_DEFAULT 'B'
#define CHARTABLE_GRAPHICS '0'

struct kterm_color {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct terminal_session {
	int master;
	int slave;
	struct process*	proc;
	struct vgaconsole* console;
	int ctrl_hold;
	int shift_hold;

	char buffer[KTERM_SESSION_BUFFER_SIZE];
	int buffer_size;
	int buffer_pos;

	char G0;
	char G1;
	int G1_active;
	int keypad_app_mode;	// \033=
	struct kterm_color foreground_color;
	struct kterm_color background_color;
};

char keyboard_get_key_ascii(int shifted, char keycode);
char keyboard_get_key_ascii_normal(char keycode);

void kterm_process_start();
void kterm_main();

void kterm_session_process(struct terminal_session* session);

char kterm_session_next_char(struct terminal_session* session);

void kterm_process_esc_sequence(struct terminal_session* session);

void kterm_session_process_csi(struct terminal_session* session);

#endif