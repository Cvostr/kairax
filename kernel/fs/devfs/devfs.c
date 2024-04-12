#include "devfs.h"
#include "list/list.h"
#include "string.h"
#include "mem/kheap.h"
#include "proc/syscalls.h"
#include "kairax/time.h"

struct inode_operations root_inode_ops;
struct file_operations  root_file_ops;
struct super_operations devfs_sb_ops;

struct inode_operations dev_inode_ops;

struct inode* devfs_root_inode;
list_t* devfs_devices;

#define DEVFS_ROOT_INODE 2
uint64_t inode_index = DEVFS_ROOT_INODE;

spinlock_t  devfs_lock;

void devfs_init()
{
    devfs_devices = create_list();

    struct timeval current_time;
    sys_get_time_epoch(&current_time);

    // Заранее создать корневую inode
    devfs_root_inode = new_vfs_inode();
    devfs_root_inode->inode = inode_index++;                   
    devfs_root_inode->mode = INODE_TYPE_DIRECTORY;
    devfs_root_inode->create_time = current_time.tv_sec;
    devfs_root_inode->access_time = current_time.tv_sec;
    devfs_root_inode->modify_time = current_time.tv_sec;
    devfs_root_inode->operations = &root_inode_ops;
    devfs_root_inode->file_ops = &root_file_ops;
    atomic_inc(&devfs_root_inode->reference_count);

    filesystem_t* devfs = new_filesystem();
    devfs->name = "devfs";
    devfs->mount = devfs_mount;

    filesystem_register(devfs);

    root_inode_ops.chmod = NULL;
    root_inode_ops.mkdir = NULL;
    root_inode_ops.mkfile = NULL;

    root_file_ops.readdir = devfs_readdir;

    devfs_sb_ops.read_inode = devfs_read_node;
    devfs_sb_ops.find_dentry = devfs_find_dentry;

    memset(&dev_inode_ops, 0, sizeof(struct inode_operations));
}

struct devfs_device* new_devfs_device_struct()
{
    struct devfs_device* result = kmalloc(sizeof(struct devfs_device));
    memset(result, 0, sizeof(struct devfs_device));
    return result;
}

struct inode* devfs_mount(drive_partition_t* drive, struct superblock* sb)
{
    devfs_root_inode->sb = sb;   

    sb->fs_info = NULL;
    sb->operations = &devfs_sb_ops;

    return devfs_root_inode;
}

int devfs_add_char_device(const char* name, struct file_operations* fops)
{
    acquire_spinlock(&devfs_lock);

    struct devfs_device* device = new_devfs_device_struct();

    struct timeval current_time;
    sys_get_time_epoch(&current_time);

    // Создание inode
    device->inode = new_vfs_inode();
    device->inode->inode = inode_index++;                   
    device->inode->mode = INODE_FLAG_CHARDEVICE;
    device->inode->sb = devfs_root_inode->sb;
    device->inode->create_time = current_time.tv_sec;
    device->inode->access_time = current_time.tv_sec;
    device->inode->modify_time = current_time.tv_sec;
    device->inode->file_ops = fops;
    device->inode->operations = &dev_inode_ops;
    atomic_inc(&device->inode->reference_count);

    // Создание dentry
    device->dentry = new_dentry();
    device->dentry->d_inode = device->inode;
    atomic_inc(&device->inode->reference_count);
    device->dentry->sb = devfs_root_inode->sb;
    strcpy(device->dentry->name, name);
    atomic_inc(&device->dentry->refs_count);
    
    // Добавление в список
    list_add(devfs_devices, device);

    release_spinlock(&devfs_lock);

    return 0;
}

struct dirent* devfs_readdir(struct file* dir, uint32_t index)
{
    struct inode* vfs_inode = dir->inode;
    if (vfs_inode->inode != DEVFS_ROOT_INODE)
        return NULL;

    if (index >= devfs_devices->size)
        return NULL;

    acquire_spinlock(&devfs_lock);

    struct devfs_device* device = (struct devfs_device*)(list_get(devfs_devices, index));

    struct dirent* result = new_vfs_dirent();
    result->inode = device->inode->inode;
    result->offset = index;
    result->type = DT_CHR;
    strcpy(result->name, device->dentry->name);

    release_spinlock(&devfs_lock);
    return result;
}

struct inode* devfs_read_node(struct superblock* sb, uint64_t ino_num)
{
    struct inode* result = NULL;

    if (ino_num == DEVFS_ROOT_INODE)
        return devfs_root_inode;

    acquire_spinlock(&devfs_lock);

    struct list_node* current = devfs_devices->head;
    struct devfs_device* device = (struct devfs_device*)current->element;

    for (size_t i = 0; i < devfs_devices->size; i++) {
        
        if (device->inode->inode == ino_num) {
            result = device->inode;
            goto exit;
        }

        current = current->next;
        device = (struct devfs_device*)current->element;
    }

exit:
    release_spinlock(&devfs_lock);
    return result;
}

uint64 devfs_find_dentry(struct superblock* sb, uint64_t parent_inode_index, const char *name, int* type)
{
    if (parent_inode_index != DEVFS_ROOT_INODE) {
        return WRONG_INODE_INDEX;
    }

    uint64_t inode = 0;
    acquire_spinlock(&devfs_lock);

    struct list_node* current = devfs_devices->head;
    struct devfs_device* device = (struct devfs_device*)current->element;

    for (size_t i = 0; i < devfs_devices->size; i++) {
        
        if (strcmp(device->dentry->name, name) == 0) {
            inode = device->inode->inode;
            goto exit;
        }

        current = current->next;
        device = (struct devfs_device*)current->element;
    }

exit:
    release_spinlock(&devfs_lock);
    return inode;
}