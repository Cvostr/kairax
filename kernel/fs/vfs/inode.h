#ifndef _INODE_H
#define _INODE_H

#include "kairax/atomic.h"
#include "kairax/types.h"
#include "sync/spinlock.h"
#include "dirent.h"
#include "stat.h"

struct inode;
struct superblock;
struct dentry;

struct inode_operations {
    int      (*chmod)(struct inode*, uint32_t);
    int      (*mkdir)(struct inode*, const char*, uint32_t);
    int      (*mkfile)(struct inode*, const char*, uint32_t);
    int      (*truncate)(struct inode*);
    int      (*rename)(struct inode*, struct dentry*, struct inode*, const char*);
    int      (*link)(struct dentry*, struct inode*, const char*);
    int      (*unlink)(struct inode*, struct dentry*);
    int      (*rmdir)(struct inode*, struct dentry*);
};

struct file_operations;

#define INODE_TYPE_MASK        0xF000
#define INODE_TYPE_FILE        0x8000
#define INODE_TYPE_DIRECTORY   0x4000
#define INODE_FLAG_CHARDEVICE  0x2000
#define INODE_FLAG_BLOCKDEVICE 0x6000
#define INODE_FLAG_PIPE        0x1000
#define INODE_FLAG_SYMLINK     0xA000
#define INODE_FLAG_SOCKET      0xC000

#define WRONG_INODE_INDEX       UINT64_MAX

#define INODE_CLOSE_SAFE(x) if (x) {inode_close(x); x = 0;}

// Представление объекта со стороны файловой системы
struct inode {
    uint32_t    mode;       // Тип и разрешения
    uint32_t    uid;        // Идентификатор пользователя, владеющего файлом
    uint32_t    gid;        // Идентификатор группы
    uint64_t    inode;      // Номер узла в драйвере файловой системы
    uint64_t    size;       // Размер файла (байт)
    uint64_t    blocks;
    uint32_t    hard_links; // количество ссылок
    dev_t       device;

    struct      superblock* sb;

    atomic_t    reference_count;

    uint64_t    create_time;
    uint64_t    access_time;
    uint64_t    modify_time;

    struct inode_operations* operations;
    struct file_operations*  file_ops;   

    spinlock_t      spinlock;

    void*       private_data;
};

int inode_chmod(struct inode* node, uint32_t mode);

void inode_open(struct inode* node, uint32_t flags);

void inode_close(struct inode* node);

int inode_stat(struct inode* node, struct stat* sstat);

int inode_mkdir(struct inode* node, const char* name, uint32_t mode);

int inode_mkfile(struct inode* node, const char* name, uint32_t mode);

int inode_truncate(struct inode* inode);

int inode_unlink(struct inode* parent, struct dentry* child);

int inode_rmdir(struct inode* parent, struct dentry* child);

int inode_rename(struct inode* parent, struct dentry* orig, struct inode* new_parent, const char* name);

#endif