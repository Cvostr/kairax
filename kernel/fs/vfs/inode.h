#ifndef _INODE_H
#define _INODE_H

#include "atomic.h"
#include "types.h"
#include "sync/spinlock.h"
#include "dirent.h"

struct inode;

typedef void      (*open_type_t)(struct inode*, uint32_t);
typedef void      (*close_type_t)(struct inode*);
typedef ssize_t  (*read_type_t)(struct inode*, uint32_t, uint32_t,char*);
typedef ssize_t  (*write_type_t)(struct inode*, uint32_t, uint32_t,char*);
typedef struct dirent* (*readdir_type_t)(struct inode*, uint32_t);
typedef struct inode*  (*finddir_type_t)(struct inode*, char*); 

typedef void      (*chmod_type_t)(struct inode*, uint32_t);
typedef void      (*mkdir_type_t)(struct inode*, char*);
typedef void      (*mkfile_type_t)(struct inode*, char*);

struct inode_operations {
    open_type_t     open;
    close_type_t    close;
    read_type_t     read;
    write_type_t    write;

    readdir_type_t  readdir;
    finddir_type_t  finddir;

    chmod_type_t    chmod;
    mkdir_type_t    mkdir;
    mkfile_type_t   mkfile;
};

#define INODE_TYPE_MASK        0xF000
#define INODE_TYPE_FILE        0x8000
#define INODE_TYPE_DIRECTORY   0x4000
#define INODE_FLAG_CHARDEVICE  0x2000
#define INODE_FLAG_BLOCKDEVICE 0x6000
#define INODE_FLAG_PIPE        0x1000
#define INODE_FLAG_SYMLINK     0xA000

// Представление объекта со стороны файловой системы
struct inode {
    uint32_t    mode;       // Тип и разрешения
    uint32_t    uid;        // Идентификатор пользователя, владеющего файлом
    uint32_t    gid;        // Идентификатор группы
    uint64_t    inode;      // Номер узла в драйвере файловой системы
    uint64_t    size;       // Размер файла (байт)
    uint32_t    hard_links; // количество ссылок

    void*       fs_d;       // Указатель на данные драйвера
    atomic_t    reference_count;

    uint64_t    create_time;
    uint64_t    access_time;
    uint64_t    modify_time;

    struct inode_operations* operations;

    spinlock_t      spinlock;
};

#endif