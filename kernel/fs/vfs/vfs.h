#ifndef _VFS_H
#define _VFS_H

#include "stdint.h"

#define MAX_NODE_NAME_LEN 256

struct fs_node;
struct dirent;

typedef void     (*open_type_t)(struct fs_node*);
typedef void     (*close_type_t)(struct fs_node*);
typedef uint32_t (*read_type_t)(struct fs_node*,uint32_t,uint32_t,char*);
typedef uint32_t (*write_type_t)(struct fs_node*,uint32_t,uint32_t,char*);
typedef struct dirent * (*readdir_type_t)(struct fs_node*,uint32_t);

typedef struct {
    open_type_t     open;
    close_type_t    close;
    read_type_t     read;
    write_type_t    write;
} node_fs_operations_t;

typedef struct PACKED {
    char        name[MAX_NODE_NAME_LEN];
    uint32_t    flags;
    uint64_t    length;
} fs_node_t;

struct
{
  char name[MAX_NODE_NAME_LEN];   // Имя файла..
  uint32_t ino;                   // Номер inode. Требеся для POSIX.
} dirent; 

#endif