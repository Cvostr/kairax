#ifndef _PROCESS_LIST_H
#define _PROCESS_LIST_H

#include "types.h"
#include "fs/vfs/dentry.h"

struct process;

pid_t process_add_to_list(struct process* process);

void get_process_count(size_t *processes_n, size_t *threads);

struct process* process_get_by_id(pid_t id);

void process_remove_from_list(struct process* process);

int process_list_is_dentry_used_as_cwd(struct dentry* dentry);

#endif