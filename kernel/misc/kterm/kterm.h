#ifndef _KTERM_H
#define _KTERM_H

#include "vgaterm.h"
#include "proc/process.h"

#define KTERM_SESSION_BUFFER_SIZE 128

struct terminal_session {
	int master;
	int slave;
	struct process*	proc;
	struct vgaconsole* console;
	int ctrl_hold;

	char buffer[KTERM_SESSION_BUFFER_SIZE];
	int buffer_size;
	int buffer_pos;
};

void kterm_process_start();
void kterm_main();

void kterm_session_process(struct terminal_session* session);

char kterm_session_next_char(struct terminal_session* session);

void kterm_process_esc_sequence(struct terminal_session* session);

void kterm_session_process_csi(struct terminal_session* session);

#endif