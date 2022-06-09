#ifndef _FILE_H
#define _FILE_H

#include "stdint.h"
#define MAX_NODE_NAME_LEN 256

struct vfs_inode;

typedef struct PACKED {
    char        name[256];
    uint64_t    inode;
    uint64_t    size;
} dirent_t;

typedef void      (*open_type_t)(struct vfs_inode*);
typedef void      (*close_type_t)(struct vfs_inode*);
typedef uint32_t  (*read_type_t)(struct vfs_inode*, uint32_t, uint32_t,char*);
typedef uint32_t  (*write_type_t)(struct vfs_inode*, uint32_t, uint32_t,char*);
typedef dirent_t* (*readdir_type_t)(struct vfs_inode*, uint32_t);
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

typedef struct vfs_inode {
    char        name[256];
    uint32_t    flags;
    uint32_t    mask;       //Разрешения
    uint32_t    uid;        // Идентификатор пользователя, владеющего файлом
    uint32_t    gid;        // Идентификатор группы
    uint64_t    inode;  // Номер узла в драйвере файловой системы
    uint64_t    size;       // Размер файла (байт)

    void*       fs_d;       // Указатель на данные драйвера

    uint64_t    create_time;
    uint64_t    access_time;
    uint64_t    modify_time;

    inode_operations_t operations;
} vfs_inode_t;

#endif