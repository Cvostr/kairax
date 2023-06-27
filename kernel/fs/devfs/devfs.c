#include "devfs.h"

void devfs_init()
{
    filesystem_t* devfs = new_filesystem();
    devfs->name = "devfs";
    devfs->mount = devfs_mount;

    filesystem_register(devfs);

    vfs_mount_fs("/dev", NULL, "devfs");
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

    result->operations.open = devfs_open;
    result->operations.chmod = NULL;
    result->operations.mkdir = NULL;
    result->operations.mkfile = NULL;
    result->operations.finddir = devfs_finddir;
    result->operations.readdir = devfs_readdir;
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