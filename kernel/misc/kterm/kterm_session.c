#include "kterm.h"
#include "proc/syscalls.h"

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
				console_print_char(session->console, c);
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

void kterm_session_process_csi(struct terminal_session* session)
{
	int args[20];
	int argc = 0;
	int questionSign = 0;
	char terminateChar = 0;

	int number = 0;

	while (1) {
		char chr = kterm_session_next_char(session);

		if (chr == ';') {

			args[argc ++] = number;
			number = 0;

		} else if ('0' <= chr && chr <= '9') {

			number = number * 10 + (chr - '0');

		} else if (0x40 <= chr && chr <= 0x7E) {
			terminateChar = chr;
			break;
		}
	}

    switch (terminateChar) {
        case 'm' :
            // SGR
            break;
    }
}