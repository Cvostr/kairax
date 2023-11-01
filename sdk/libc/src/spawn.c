#include "spawn.h"
#include "process.h"
#include "fcntl.h"

int posix_spawn(pid_t* pid_ptr,
                       const char* path,
                       //const posix_spawn_file_actions_t* actions,
                       //const posix_spawnattr_t* attr,
                       char* const argv[]
                       //char* const env[]
                       )
{
    int argc = 0;
    char** arg = argv;
    while (*arg) {
        argc ++;
        arg++;
    }

    struct process_create_info info;
    info.current_directory = NULL;
    info.num_args = argc;
    info.args = argv;
    info._stdout = 1;
    info._stdin = -1;
    info._stderr = -1;
    pid_t rc = create_process(AT_FDCWD, path, &info);

    if (pid_ptr) {
        *pid_ptr = rc;
    }
}