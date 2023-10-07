#ifndef _SYS_FILES_H
#define _SYS_FILES_H

#include "sys/types.h"
#include "stddef.h"
#include "sys/stat.h"
#include <stdint.h>
#include "dirent.h"

#define FILE_OPEN_FLAG_TRUNCATE     00001000
#define FILE_OPEN_FLAG_APPEND       00002000

int stat_at(int dirfd, const char* filepath, struct stat* st);

int readdir(int fd, struct dirent* direntry);

int chmod_at(int dirfd, const char* filepath, mode_t mode);

#endif