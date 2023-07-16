#include "devfs.h"
#include "list/list.h"

struct inode_operations root_inode_ops;
struct super_operations devfs_sb_ops;

struct inode* devfs_root_inode;
list_t* devfs_inodes;

void devfs_init()
{
    devfs_inodes = create_list();

    // Заранее создать корневую inode
    devfs_root_inode = new_vfs_inode();
    devfs_root_inode->inode = 0;                   
    devfs_root_inode->mode = INODE_TYPE_DIRECTORY;
    devfs_root_inode->create_time = 0;
    devfs_root_inode->access_time = 0;
    devfs_root_inode->modify_time = 0;
    devfs_root_inode->operations = &root_inode_ops;

    // Добавить корневую в список inode файловой системы
    list_add(devfs_inodes, devfs_root_inode);

    filesystem_t* devfs = new_filesystem();
    devfs->name = "devfs";
    devfs->mount = devfs_mount;

    filesystem_register(devfs);

    //vfs_mount_fs("/dev", NULL, "devfs");

    root_inode_ops.open = devfs_open;
    root_inode_ops.chmod = NULL;
    root_inode_ops.mkdir = NULL;
    root_inode_ops.mkfile = NULL;
    root_inode_ops.readdir = devfs_readdir;

    devfs_sb_ops.read_inode = devfs_read_node;
    devfs_sb_ops.find_dentry = devfs_find_dentry;
}

struct inode* devfs_mount(drive_partition_t* drive, struct superblock* sb)
{
    devfs_root_inode->sb = sb;   

    sb->fs_info = NULL;
    sb->operations = &devfs_sb_ops;

    return devfs_root_inode;
}

void devfs_open(struct inode* inode, uint32_t flags)
{

}

struct dirent* devfs_readdir(struct inode* dir, uint32_t index)
{
    return NULL;
}

struct inode* devfs_read_node(struct superblock* sb, uint64_t ino_num)
{
    struct inode* result = NULL;
    
    struct list_node* current = devfs_inodes->head;
    struct inode* inode = (struct inode*)current->element;

    for (size_t i = 0; i < devfs_inodes->size; i++) {
        
        if (inode->inode == ino_num) {
            result = inode;
            goto exit;
        }

        current = current->next;
        inode = (struct inode*)current->element;
    }

exit:
    return result;
}

uint64 devfs_find_dentry(struct superblock* sb, uint64_t parent_inode_index, const char *name)
{
    return 0;
}