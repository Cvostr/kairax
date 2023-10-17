#ifndef _PROCESS_LIST_H
#define _PROCESS_LIST_H

#include "types.h"

struct process;

pid_t process_add_to_list(struct process* process);

struct process* process_get_by_id(pid_t id);

void process_remove_from_list(struct process* process);

#endif