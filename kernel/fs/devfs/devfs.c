#include "devfs.h"

void devfs_init()
{
    filesystem_t* devfs = new_filesystem();
    devfs->name = "devfs";
    devfs->mount = devfs_mount;

    filesystem_register(devfs);

    vfs_mount_fs("/dev", NULL, "devfs");
}

vfs_inode_t* devfs_mount(drive_partition_t* drive)
{
    vfs_inode_t* result = new_vfs_inode();
    result->inode = 2;              
    result->mask = 0;
    result->fs_d = 0;        
    result->flags = INODE_FLAG_DIRECTORY;

    result->create_time = 0;
    result->access_time = 0;
    result->modify_time = 0;

    result->operations.open = devfs_open;
    result->operations.chmod = NULL;
    result->operations.mkdir = NULL;
    result->operations.mkfile = NULL;
    result->operations.finddir = devfs_finddir;
    result->operations.readdir = devfs_readdir;
}

void devfs_open(vfs_inode_t* inode, uint32_t flags)
{

}

vfs_inode_t* devfs_readdir(vfs_inode_t* dir, uint32_t index)
{
    return NULL;
}

vfs_inode_t* devfs_finddir(vfs_inode_t* parent, char *name)
{
    return parent; // без вложений, вернем корень
}