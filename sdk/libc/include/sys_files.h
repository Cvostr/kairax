#ifndef _SYS_FILES_H
#define _SYS_FILES_H

#include "sys/types.h"
#include "stddef.h"
#include "sys/stat.h"
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

#define DIRFD_IS_FD  1

// user permissions
#define PERM_USER_READ  00400
#define PERM_USER_WRITE 00200
#define PERM_USER_EXEC  00100

// group permissions
#define PERM_GROUP_READ 00040
#define PERM_GROUP_WRITE 00020
#define PERM_GROUP_EXEC  00010

// others permissions
#define PERM_OTHERS_READ    00004
#define PERM_OTHERS_WRITE   00002
#define PERM_OTHERS_EXEC    00001

int open_file_at(int dirfd, const char* filepath, int flags, int mode);

int open_file(const char* filepath, int flags, int mode);

int mkdir(const char* dirpath, int mode);

int close(int fd);

ssize_t read(int fd, char* buffer, size_t size);

ssize_t write(int fd, const char* buffer, size_t size);

int fstat(int fd, struct stat* st);

int stat(const char* filepath, struct stat* st);

int stat_at(int dirfd, const char* filepath, struct stat* st);

int readdir(int fd, struct dirent* direntry);

off_t file_seek(int fd, off_t offset, int whence);

int ioctl(int fd, uint64_t request, uint64_t arg);

int chmod(const char* filepath, mode_t mode);

int chmod_at(int dirfd, const char* filepath, mode_t mode);

#endif