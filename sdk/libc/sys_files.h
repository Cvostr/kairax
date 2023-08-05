#ifndef _SYS_FILES_H
#define _SYS_FILES_H

#include "types.h"
#include "stddef.h"
#include "stat.h"
#include "dirent.h"

#define FILE_OPEN_MODE_READ_ONLY    00000001
#define FILE_OPEN_MODE_WRITE_ONLY   00000002
#define FILE_OPEN_MODE_READ_WRITE   00000003

#define FILE_OPEN_FLAG_CREATE       00000100
#define FILE_OPEN_FLAG_TRUNCATE     00001000
#define FILE_OPEN_FLAG_APPEND       00002000
#define FILE_OPEN_FLAG_DIRECTORY    00200000

#define lseek file_seek

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define FD_CWD		-2

int open_file(const char* filepath, int flags, int mode);

int close(int fd);

ssize_t read(int fd, char* buffer, size_t size);

ssize_t write(int fd, const char* buffer, size_t size);

int fdstat(int fd, struct stat* st);

int file_stat(const char* filepath, struct stat* st);

int readdir(int fd, struct dirent* direntry);

off_t file_seek(int fd, off_t offset, int whence);

#endif