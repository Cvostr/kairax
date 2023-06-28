#ifndef _DENTRY_H
#define _DENTRY_H

#include "inode.h"
#include "list/list.h"
#include "dirent.h"

struct dentry {
    struct inode*   inode;
    struct dentry*  parent;
    char            name[MAX_DIRENT_NAME_LEN];
    list_t *        subdirs;
};

struct dentry* new_dentry();

void free_dentry(struct dentry* dentry);

struct dentry* dentry_get_child_with_name(struct dentry* parent, const char* child_name);

#endif