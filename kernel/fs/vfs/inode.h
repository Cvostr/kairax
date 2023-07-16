#ifndef _INODE_H
#define _INODE_H

#include "atomic.h"
#include "types.h"
#include "sync/spinlock.h"
#include "dirent.h"
#include "stat.h"

struct inode;
struct superblock;

struct inode_operations {
    void      (*open)(struct inode*, uint32_t);
    void      (*close)(struct inode*);
    ssize_t   (*read)(struct inode*, off_t, size_t, char*);
    ssize_t   (*write)(struct inode*, off_t, size_t, const char*);

    struct dirent* (*readdir)(struct inode*, uint32_t);

    int      (*chmod)(struct inode*, uint32_t);
    int      (*mkdir)(struct inode*, const char*, uint32_t);
    void      (*mkfile)(struct inode*, char*);
};

#define INODE_TYPE_MASK        0xF000
#define INODE_TYPE_FILE        0x8000
#define INODE_TYPE_DIRECTORY   0x4000
#define INODE_FLAG_CHARDEVICE  0x2000
#define INODE_FLAG_BLOCKDEVICE 0x6000
#define INODE_FLAG_PIPE        0x1000
#define INODE_FLAG_SYMLINK     0xA000

#define WRONG_INODE_INDEX       UINT64_MAX

// Представление объекта со стороны файловой системы
struct inode {
    uint32_t    mode;       // Тип и разрешения
    uint32_t    uid;        // Идентификатор пользователя, владеющего файлом
    uint32_t    gid;        // Идентификатор группы
    uint64_t    inode;      // Номер узла в драйвере файловой системы
    uint64_t    size;       // Размер файла (байт)
    uint32_t    hard_links; // количество ссылок

    struct superblock* sb;

    atomic_t    reference_count;

    uint64_t    create_time;
    uint64_t    access_time;
    uint64_t    modify_time;

    struct inode_operations* operations;

    spinlock_t      spinlock;
};

ssize_t inode_read(struct inode* node, loff_t* offset, size_t size, char* buffer);

ssize_t inode_write(struct inode* node, loff_t* offset, size_t size, const char* buffer);

struct dirent* inode_readdir(struct inode* node, uint32_t index);

void inode_chmod(struct inode* node, uint32_t mode);

void inode_open(struct inode* node, uint32_t flags);

void inode_close(struct inode* node);

void inode_stat(struct inode* node, struct stat* sstat);

int inode_mkdir(struct inode* node, const char* name, uint32_t mode);

#endif