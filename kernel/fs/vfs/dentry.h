#ifndef _DENTRY_H
#define _DENTRY_H

#include "inode.h"
#include "sync/spinlock.h"
#include "list/list.h"
#include "dirent.h"
#include "stdc/atomic.h"

struct superblock;

struct dentry {
    uint64_t            inode;
    struct dentry*      parent;
    struct superblock*  sb;
    char                name[MAX_DIRENT_NAME_LEN];
    list_t *            subdirs;
    spinlock_t          lock;
    atomic_t            refs_count;
};

struct dentry* new_dentry();

void free_dentry(struct dentry* dentry);

void close_dentry(struct dentry* dentry);

void dentry_open(struct dentry* dentry);

void dentry_add_subdir(struct dentry* parent, struct dentry* dir);

void dentry_remove_subdir(struct dentry* parent, struct dentry* dir);

struct dentry* dentry_get_child_with_name(struct dentry* parent, const char* child_name);

#endif