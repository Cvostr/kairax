#include "kterm.h"
#include "proc/syscalls.h"
#include "stdio.h"

void kterm_session_process(struct terminal_session* session)
{
    char c = kterm_session_next_char(session);

	if (c != -1) {
		switch (c) {
			case '\b':
				console_backspace(session->console, 1);
				break;
			case '\r':
				console_cr(session->console);
				break;
			case '\n':
				console_lf(session->console);
				break;
			case '\033':
				kterm_process_esc_sequence(session);
				break;
			default:
				console_print_char(session->console, c,
				 	session->foreground_color.r,
				 	session->foreground_color.g,
				 	session->foreground_color.b);
		}
	}
}

char kterm_session_next_char(struct terminal_session* session)
{
	if (session->buffer_size == 0 || session->buffer_size == session->buffer_pos) {
		session->buffer_size = sys_read_file(session->master, session->buffer, KTERM_SESSION_BUFFER_SIZE);
		session->buffer_pos = 0;
	}

	if (session->buffer_size == 0) {
		return -1;
	}

	return session->buffer[session->buffer_pos++];
}

void kterm_process_esc_sequence(struct terminal_session* session)
{
	char seqchr = kterm_session_next_char(session);

	switch (seqchr) {
		case '[':
			kterm_session_process_csi(session);
			break;
		case 'E':
			console_cr(session->console);
			console_lf(session->console);
			break;
	}
}

// https://en.wikipedia.org/wiki/ANSI_escape_code#CSI

#define CSI_MAX_ARGS 20

#define SGR 'm'
#define DECSED 'J'

void kterm_session_process_csi(struct terminal_session* session)
{
	int args[CSI_MAX_ARGS];
	int argc = 0;
	int questionSign = 0;
	char terminateChar = 0;

	int number = 0;

	while (1) {
		char chr = kterm_session_next_char(session);

		switch (chr) {
			case '0' ... '9':
				number = number * 10 + (chr - '0');
				break;
			break;
			case 0x40 ... 0x7E:
				terminateChar = chr;
			case ';':
				if (argc < CSI_MAX_ARGS) {
					args[argc ++] = number;
					number = 0;
				}
				break;
		}

		if (terminateChar != 0) {
			break;
		}
	}

	struct kterm_color colors[] = {
		{.r = 0, .g = 0, .b = 0},
		{.r = 170, .g = 0, .b = 0},
		{.r = 0, .g = 170, .b = 0},	// green
		{.r = 170, .g = 85, .b = 0}, //yellow
		{.r = 0, .g = 0, .b = 170},	// blue
		{.r = 170, .g = 0, .b = 170},
		{.r = 0, .g = 170, .b = 170},
		{.r = 170, .g = 170, .b = 170}, //white
		{.r = 85, .g = 85, .b = 85},	// bright black
		{.r = 255, .g = 85, .b = 85},	//bright red
		{.r = 85, .g = 255, .b = 85},	// bright green
		{.r = 255, .g = 255, .b = 85},	// bright yellow
		{.r = 85, .g = 85, .b = 255},	// bright blue
		{.r = 255, .g = 85, .b = 255},	// bright magenta
		{.r = 85, .g = 255, .b = 255},
		{.r = 255, .g = 255, .b = 255},
	};

    switch (terminateChar) {
        case SGR :
			for (int i = 0; i < argc; i ++) {
				switch (args[i]) {
					case 0:
						struct kterm_color foreground_color = DEFAULT_FOREGROUND_COLOR;
						session->foreground_color = foreground_color;
						break;
					case 30 ... 37:
						session->foreground_color = colors[args[i] - 30];
						break;
					case 90 ... 97:
						session->foreground_color = colors[args[i] - 82];
						break;
				}
			}

            break;
		case DECSED:
			int mode = args[0];
			switch (mode) {
				case 2:
					console_clear(session->console);
					break;
				default:
					printk("Unknown DECSED mode %i\n", mode);
			}
			break;
    }
}