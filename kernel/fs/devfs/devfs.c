#include "devfs.h"

struct inode_operations root_inode_ops;

void devfs_init()
{
    filesystem_t* devfs = new_filesystem();
    devfs->name = "devfs";
    devfs->mount = devfs_mount;

    filesystem_register(devfs);

    vfs_mount_fs("/dev", NULL, "devfs");

    root_inode_ops.open = devfs_open;
    root_inode_ops.chmod = NULL;
    root_inode_ops.mkdir = NULL;
    root_inode_ops.mkfile = NULL;
    root_inode_ops.finddir = devfs_finddir;
    root_inode_ops.readdir = devfs_readdir;
}

struct inode* devfs_mount(drive_partition_t* drive)
{
    struct inode* result = new_vfs_inode();
    result->inode = 2;              
    result->fs_d = 0;        
    result->mode = INODE_TYPE_DIRECTORY;

    result->create_time = 0;
    result->access_time = 0;
    result->modify_time = 0;

    result->operations = &root_inode_ops;
}

void devfs_open(struct inode* inode, uint32_t flags)
{

}

struct dirent* devfs_readdir(struct inode* dir, uint32_t index)
{
    return NULL;
}

struct inode* devfs_finddir(struct inode* parent, char *name)
{
    return parent; // без вложений, вернем корень
}