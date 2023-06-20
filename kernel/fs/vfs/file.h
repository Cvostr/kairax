#ifndef _FILE_H
#define _FILE_H

#include "dirent.h"
#include "sync/spinlock.h"
#include "atomic.h"

struct vfs_inode;

typedef void      (*open_type_t)(struct vfs_inode*, uint32_t);
typedef void      (*close_type_t)(struct vfs_inode*);
typedef uint32_t  (*read_type_t)(struct vfs_inode*, uint32_t, uint32_t,char*);
typedef uint32_t  (*write_type_t)(struct vfs_inode*, uint32_t, uint32_t,char*);
typedef struct vfs_inode* (*readdir_type_t)(struct vfs_inode*, uint32_t);
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

#define VFS_FLAG_FILE        0x01
#define VFS_FLAG_DIRECTORY   0x02
#define VFS_FLAG_CHARDEVICE  0x04
#define VFS_FLAG_BLOCKDEVICE 0x08
#define VFS_FLAG_PIPE        0x10
#define VFS_FLAG_SYMLINK     0x20
#define VFS_FLAG_MOUNTPOINT  0x40

// Представление объекта со стороны файловой системы
typedef struct PACKED {
    char        name[MAX_PATH_LEN];
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

typedef size_t loff_t;

#define FILE_OPEN_MODE_READ_ONLY    0b01
#define FILE_OPEN_MODE_WRITE_ONLY   0b10
#define FILE_OPEN_MODE_READ_WRITE   0b11

// Открытый файл
typedef struct PACKED {
    vfs_inode_t*    inode;
    int             mode;
    int             flags;
    loff_t          pos;
    void*           private_data;
    spinlock_t      spinlock;
} file_t;

#endif