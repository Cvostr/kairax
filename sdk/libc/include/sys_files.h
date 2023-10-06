#ifndef _SYS_FILES_H
#define _SYS_FILES_H

#include "sys/types.h"
#include "stddef.h"
#include "sys/stat.h"
#include "dirent.h"

#define FILE_OPEN_MODE_READ_ONLY    00000000
#define FILE_OPEN_MODE_WRITE_ONLY   00000001
#define FILE_OPEN_MODE_READ_WRITE   00000002

#define FILE_OPEN_FLAG_CREATE       00000100
#define FILE_OPEN_FLAG_TRUNCATE     00001000
#define FILE_OPEN_FLAG_APPEND       00002000
#define FILE_OPEN_FLAG_DIRECTORY    00200000

#define FD_CWD		-2

int open_file(const char* filepath, int flags, int mode);

int stat_at(int dirfd, const char* filepath, struct stat* st);

int readdir(int fd, struct dirent* direntry);

int chmod_at(int dirfd, const char* filepath, mode_t mode);

#endif