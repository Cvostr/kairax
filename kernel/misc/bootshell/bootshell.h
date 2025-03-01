#ifndef _BOOTSHELL_H
#define _BOOTSHELL_H

struct process;

struct process* bootshell_spawn_new_process(struct process* parent);
void bootshell();

#endif