#ifndef _PROCESS_LIST_H
#define _PROCESS_LIST_H

#include "types.h"
#include "fs/vfs/dentry.h"

#define MAX_PROCESSES 3000

struct process;

// добавить процесс в список.
// при этом будет сгенерирован PID, записан в структуру
// также увеличен счетчик ссылок
pid_t process_add_to_list(struct process* process);
void get_process_count(size_t *processes_n, size_t *threads);
// получить процесс по pid с увеличением счетчика ссылок
struct process* process_get_by_id(pid_t id);
void process_remove_from_list(struct process* process);
int process_list_is_dentry_used_as_cwd(struct dentry* dentry);
int process_list_send_signal_pg(pid_t pg, int signal);

#endif