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
    int      (*mknod)(struct inode*, const char*, mode_t);
    int      (*symlink)(struct inode*, const char*, const char*);
    int      (*set_datetime)(struct inode*, const struct timespec*, const struct timespec*);
    ssize_t  (*readlink)(struct inode*, char*, size_t);
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

#define INODE_MODE_SETUID      04000
#define INODE_MODE_SETGID      02000

#define S_IRWXU	    00700
#define S_IRUSR	    00400	
#define S_IWUSR	    00200	
#define S_IXUSR	    00100
#define S_IRWXG	    00070	
#define S_IRGRP	    00040	
#define S_IWGRP	    00020	
#define S_IXGRP	    00010	
#define S_IRWXO	    00007
#define S_IROTH	    00004	
#define S_IWOTH	    00002	
#define S_IXOTH	    00001	

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

    struct timespec    create_time;
    struct timespec    access_time;
    struct timespec    modify_time;

    struct inode_operations* operations;
    struct file_operations*  file_ops;   

    spinlock_t      spinlock;

    void*       private_data;
};

struct inode* new_vfs_inode();

int inode_check_perm(struct inode* ino, uid_t uid, gid_t gid, int ubit, int gbit, int obit);

void inode_open(struct inode* node, uint32_t flags);
void inode_close(struct inode* node);

int inode_stat(struct inode* node, struct stat* sstat);
int inode_mkdir(struct inode* node, const char* name, uint32_t mode);
int inode_mkfile(struct inode* node, const char* name, uint32_t mode);
int inode_truncate(struct inode* inode);
int inode_unlink(struct inode* parent, struct dentry* child);
int inode_linkat(struct dentry* src, struct inode* dst, const char* name);
int inode_rmdir(struct inode* parent, struct dentry* child);
int inode_rename(struct inode* parent, struct dentry* orig, struct inode* new_parent, const char* name);
int inode_mknod(struct inode* parent, const char* name, mode_t mode);
int inode_symlink(struct inode* parent, const char* name, const char* target);
int inode_chmod(struct inode* node, uint32_t mode);
int inode_set_time(struct inode* node, const struct timespec *atime, const struct timespec *mtime);
ssize_t inode_readlink(struct inode* symlink, char* buf, size_t buflen);

#endif