#include "kterm.h"
#include "proc/syscalls.h"
#include "stdio.h"
#include "ctype.h"
#include "string.h"

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
			case ESC:
				kterm_process_esc_sequence(session);
				break;
			case 0x0E:
				//printk("G1 activated\n");
				session->G1_active = TRUE;
				break;
			case 0x0F:
				//printk("G0 activated\n");
				session->G1_active = FALSE;
				break;
			default:
				char chartable = session->G1_active ? session->G1 : session->G0;
				//if (chartable == CHARTABLE_GRAPHICS)
				//	return;
				console_print_char(session->console, chartable == CHARTABLE_GRAPHICS, c,
				 	session->foreground_color.r,
				 	session->foreground_color.g,
				 	session->foreground_color.b,
					session->background_color.r,
					session->background_color.g,
					session->background_color.b);
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
		case '(':
			session->G0 = kterm_session_next_char(session);
			//printk("GO set to %c \n", session->G0);
			break;
		case ')':
			session->G1 = kterm_session_next_char(session);
			//printk("G1 set to %c \n", session->G1);
			break;
		case '=':
			session->keypad_app_mode = TRUE;
			//printk("Enabled app mode\n");
			break;
		case '>':
			session->keypad_app_mode = FALSE;
			break;
		case 'M':
			console_cursor_move_scroll(session->console, -1);
			break;
		case 'D':
			console_cursor_move_scroll(session->console, 1);
			break;
		case '7':
			session->saved_cursor_row = session->console->console_lines;
			session->saved_cursor_col = session->console->console_col;
			break;
		case '8':
			console_set_cursor_pos(session->console, session->saved_cursor_row, session->saved_cursor_col);
			break;
		default:
			printk("Unknown ESC chr %i (%c)\n", seqchr, seqchr);
			break;
	}
}

void kterm_session_reset_default_colors(struct terminal_session* session)
{
	struct kterm_color foreground_color = DEFAULT_FOREGROUND_COLOR;
	session->foreground_color = foreground_color;
	session->background_color.r = 0;
	session->background_color.g = 0;
	session->background_color.b = 0;
}

void kterm_session_swap_colors(struct terminal_session* session)
{
	struct kterm_color temp;
	memcpy(&temp, &session->foreground_color, sizeof(struct kterm_color));

	memcpy(&session->foreground_color, &session->background_color, sizeof(struct kterm_color));
	memcpy(&session->background_color, &temp, sizeof(struct kterm_color));
}

// https://en.wikipedia.org/wiki/ANSI_escape_code#CSI

#define CSI_MAX_ARGS 20

#define SGR 'm'
#define DECSED 'J'

void kterm_session_set_scroll_region(struct terminal_session* session, int argc, int args[])
{
	//printk("SCROLL args(%i) (%i %i)\n", argc, args[0], args[1]);
	int top = args[0];
	int bottom = args[1];

	if (argc == 0)
	{
		top = 0; 
		bottom = surface_get_rows();
	} 
	else if (argc == 1)
	{
		top = args[0];
		bottom = surface_get_rows();
	}

	if (bottom == 0)
		bottom = surface_get_rows();

	console_set_cursor_pos(session->console, top - 1, 0);

	console_set_scroll_region(session->console, top - 1, bottom);
}

void kterm_session_toggle(struct terminal_session* session, int argc, int args[], int dec, char terminate)
{
	// TODO: implement
	if (dec == TRUE)
	{
		if (args[0] == 7)
		{
			session->console->autowrap = (terminate == 'h');
			return;
		} 
		else if (args[0] == 25)
		{
			// TODO: implement hide cursor
			return;
		}
	}

	//printk("SCI %c DEC = %i with args(%i) (%i %i %i)\n", terminate, dec, argc, args[0], args[1], args[2]);
}

void kterm_session_process_csi(struct terminal_session* session)
{
	int mode;
	int args[CSI_MAX_ARGS];
	int argc = 0;
	int questionSign = 0;
	char terminateChar = 0;

	int number = 0;

	while (1) {
		char chr = kterm_session_next_char(session);

		switch (chr) {
			case '?':
				questionSign = 1;
				break;
			case '0' ... '9':
				number = number * 10 + (chr - '0');
				break;
			case 0x40 ... 0x7E:
				terminateChar = chr;
				// не делаем break намеренно чтобы добавить последний аргумент 
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

    switch (terminateChar) 
	{
        case SGR:
			if (argc == 0)
			{
				kterm_session_reset_default_colors(session);
				break;
			}

			for (int i = 0; i < argc; i ++) {
				switch (args[i]) {
					case 0:
						kterm_session_reset_default_colors(session);
						break;
					case 30 ... 37:
						session->foreground_color = colors[args[i] - 30];
						break;
					case 40 ... 47:
						session->background_color = colors[args[i] - 40];
						break;
					case 90 ... 97:
						session->foreground_color = colors[args[i] - 82];
						break;
					case 100 ... 107:
						session->background_color = colors[args[i] - 92];
						break;
					case 1:
						// TODO: Bold
						break;
					case 7:
						kterm_session_swap_colors(session);
						break;
					default:
						printk("Unknown SGR arg %i\n", args[i]);
						break;
				}
			}

            break;
		case DECSED:
			mode = (argc == 0) ? 0 : args[0];
			switch (mode) {
				case 0:
					console_clear_to_end(session->console);
					break;
				case 2:
					console_clear(session->console);
					break;
				default:
					printk("Unknown DECSED mode %i\n", mode);
			}
			break;
		case 'K':
			mode = (argc == 0) ? 0 : args[0];
			switch (mode)
			{
			case 0:
				console_clear_line_to_end(session->console);
				break;
			case 1:
				console_clear_line_to_begin(session->console);
				break;
			default:
				printk("Unknown DECSEL mode %i\n", mode);
				break;
			}
			break;
		case 'f':
		case 'H':
			if (argc == 0)
				console_set_cursor_pos(session->console, 0, 0);
			else if (argc == 1)
				console_set_cursor_pos(session->console, args[0] - 1, 0);
			else if (argc == 2)
				console_set_cursor_pos(session->console, args[0] - 1, args[1] - 1);
			break;
		case 'A':
			if (argc == 0 || args[0] == 0) args[0] = 1;
			console_cursor_move(session->console, 0, -args[0]);
			break;
		case 'B':
			if (argc == 0 || args[0] == 0) args[0] = 1;
			console_cursor_move(session->console, 0, args[0]);
			break;
		case 'C':
			if (argc == 0 || args[0] == 0) args[0] = 1;
			console_cursor_move(session->console, args[0], 0);
			break;
		case 'D':
			if (argc == 0 || args[0] == 0) args[0] = 1;
			console_cursor_move(session->console, -args[0], 0);
			break;
		case 'P':
			// если аргументов не было или аргумент равен 0, считаем его как 1
			if (argc == 0 || args[0] == 0) args[0] = 1;
			console_del_chars(session->console, args[0]);
			break;
		case 'l':
		case 'h':
			kterm_session_toggle(session, argc, args, questionSign, terminateChar);
			break;
		case 'r':
			kterm_session_set_scroll_region(session, argc, args);
			break;
		default:
			printk("Unknown CSI terminate chr (%c)\n", terminateChar);
    }
}