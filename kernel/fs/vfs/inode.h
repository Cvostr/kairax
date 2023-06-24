#ifndef _INODE_H
#define _INODE_H

#include "atomic.h"
#include "types.h"
#include "sync/spinlock.h"
#include "dirent.h"

struct vfs_inode;

typedef void      (*open_type_t)(struct vfs_inode*, uint32_t);
typedef void      (*close_type_t)(struct vfs_inode*);
typedef ssize_t  (*read_type_t)(struct vfs_inode*, uint32_t, uint32_t,char*);
typedef ssize_t  (*write_type_t)(struct vfs_inode*, uint32_t, uint32_t,char*);
typedef struct dirent_t* (*readdir_type_t)(struct vfs_inode*, uint32_t);
typedef struct vfs_inode*  (*finddir_type_t)(struct vfs_inode*, char*); 

typedef void      (*chmod_type_t)(struct vfs_inode*, uint32_t);
typedef void      (*mkdir_type_t)(struct vfs_inode*, char*);
typedef void      (*mkfile_type_t)(struct vfs_inode*, char*);

typedef struct inode_operations {
    open_type_t     open;
    close_type_t    close;
    read_type_t     read;
    write_type_t    write;

    readdir_type_t  readdir;
    finddir_type_t  finddir;

    chmod_type_t    chmod;
    mkdir_type_t    mkdir;
    mkfile_type_t   mkfile;
} inode_operations_t;

#define INODE_FLAG_FILE        0x01
#define INODE_FLAG_DIRECTORY   0x02
#define INODE_FLAG_CHARDEVICE  0x04
#define INODE_FLAG_BLOCKDEVICE 0x08
#define INODE_FLAG_PIPE        0x10
#define INODE_FLAG_SYMLINK     0x20
#define INODE_FLAG_MOUNTPOINT  0x40

// Представление объекта со стороны файловой системы
typedef struct PACKED {
    uint32_t    flags;
    uint32_t    mask;       //Разрешения
    uint32_t    uid;        // Идентификатор пользователя, владеющего файлом
    uint32_t    gid;        // Идентификатор группы
    uint64_t    inode;      // Номер узла в драйвере файловой системы
    uint64_t    size;       // Размер файла (байт)
    uint32_t    hard_links;

    void*       fs_d;       // Указатель на данные драйвера
    atomic_t    reference_count;

    uint64_t    create_time;
    uint64_t    access_time;
    uint64_t    modify_time;

    inode_operations_t operations;

    spinlock_t      spinlock;
} vfs_inode_t;

#endif