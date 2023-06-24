#include "vfs.h"
#include "mem/kheap.h"

#define MAX_HOLD_INODES 200

vfs_inode_t** hold_inodes = NULL;
spinlock_t vfs_list_lock;

void init_vfs_holder()
{
    size_t mem_size = MAX_HOLD_INODES * sizeof(vfs_inode_t*);
    hold_inodes = kmalloc(mem_size);
    memset(hold_inodes, 0, mem_size);
}

vfs_inode_t* vfs_get_inode_by_path(const char* path)
{
    acquire_spinlock(&vfs_list_lock);

    for (int i = 0; i < MAX_HOLD_INODES; i ++) {
        vfs_inode_t* inode = hold_inodes[i];
        if(inode != NULL) {
           // if (strcmp(inode->name, path) == 0) {
           //     return inode;
           // }
        }
    }

    release_spinlock(&vfs_list_lock);

    return NULL;
}

void vfs_hold_inode(vfs_inode_t* inode)
{
    acquire_spinlock(&vfs_list_lock);

    for (int i = 0; i < MAX_HOLD_INODES; i ++) {
        if(hold_inodes[i] == NULL) {
            hold_inodes[i] = inode;
        }
    }

    release_spinlock(&vfs_list_lock);
}

void vfs_unhold_inode(vfs_inode_t* inode)
{
    acquire_spinlock(&vfs_list_lock);
    
    for (int i = 0; i < MAX_HOLD_INODES; i ++) {
        if(hold_inodes[i] == inode) {
            hold_inodes[i] = NULL;
        }
    }

    release_spinlock(&vfs_list_lock);
}