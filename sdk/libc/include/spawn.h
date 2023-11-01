#ifndef _SPAWN_H
#define _SPAWN_H

#include "sys/types.h"

#ifdef __cplusplus
extern "C" {
#endif

int posix_spawn(pid_t* pid_ptr,
                       const char* path,
                       //const posix_spawn_file_actions_t* actions,
                       //const posix_spawnattr_t* attr,
                       char* const argv[]
                       //char* const env[]
                       );

#ifdef __cplusplus
}
#endif

#endif