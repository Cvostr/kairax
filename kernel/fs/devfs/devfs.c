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

#define DEVFS_INODE_DEFAULT_PERM 0666
#define DEVFS_ROOT_INODE_DEFAULT_PERM   0755

spinlock_t  devfs_lock;

void arch_get_timespec(struct timespec *ts);
int devfs_add_shm_dir();

void devfs_init()
{
    devfs_devices = create_list();

    struct timespec current_time;
    arch_get_timespec(&current_time);

    // Заранее создать корневую inode
    devfs_root_inode = new_vfs_inode();
    devfs_root_inode->inode = inode_index++;                   
    devfs_root_inode->mode = INODE_TYPE_DIRECTORY | DEVFS_ROOT_INODE_DEFAULT_PERM;
    devfs_root_inode->create_time = current_time;
    devfs_root_inode->access_time = current_time;
    devfs_root_inode->modify_time = current_time;
    devfs_root_inode->operations = &root_inode_ops;
    devfs_root_inode->file_ops = &root_file_ops;
    devfs_root_inode->hard_links = 1;
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

    devfs_add_shm_dir();
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

struct devfs_device* devfs_find_dev(const char* name)
{
    acquire_spinlock(&devfs_lock);

    struct list_node* current = devfs_devices->head;
    struct devfs_device* device = NULL;
    struct devfs_device* result = NULL;

    while (current != NULL) {
        
        device = (struct devfs_device*)current->element;

        if (strcmp(device->dentry->name, name) == 0) {
            result = device;
            goto exit;
        }

        current = current->next;
    }

exit:
    release_spinlock(&devfs_lock);
    return result;
}

int devfs_add_char_device(const char* name, struct file_operations* fops, void* private_data)
{
    if (devfs_find_dev(name) != NULL) {
        return ERROR_ALREADY_EXISTS;
    }

    acquire_spinlock(&devfs_lock);

    struct devfs_device* device = new_devfs_device_struct();

    struct timespec current_time;
    arch_get_timespec(&current_time);

    // Создание inode
    device->inode = new_vfs_inode();
    device->inode->inode = inode_index++;                   
    device->inode->mode = INODE_FLAG_CHARDEVICE | DEVFS_INODE_DEFAULT_PERM;
    device->inode->sb = devfs_root_inode->sb;
    device->inode->create_time = current_time;
    device->inode->access_time = current_time;
    device->inode->modify_time = current_time;
    device->inode->file_ops = fops;
    device->inode->operations = &dev_inode_ops;
    device->inode->hard_links = 1;
    device->inode->private_data = private_data;
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

int devfs_add_shm_dir()
{
    acquire_spinlock(&devfs_lock);

    struct devfs_device* device = new_devfs_device_struct();

    struct timespec current_time;
    arch_get_timespec(&current_time);

    // Создание inode
    device->inode = new_vfs_inode();
    device->inode->inode = inode_index++;                   
    device->inode->mode = INODE_TYPE_DIRECTORY | DEVFS_INODE_DEFAULT_PERM;
    device->inode->sb = devfs_root_inode->sb;
    device->inode->create_time = current_time;
    device->inode->access_time = current_time;
    device->inode->modify_time = current_time;
    device->inode->operations = &dev_inode_ops;
    device->inode->hard_links = 1;
    atomic_inc(&device->inode->reference_count);

    // Создание dentry
    device->dentry = new_dentry();
    device->dentry->d_inode = device->inode;
    atomic_inc(&device->inode->reference_count);
    device->dentry->sb = devfs_root_inode->sb;
    strcpy(device->dentry->name, "shm");
    atomic_inc(&device->dentry->refs_count);
    
    // Добавление в список
    list_add(devfs_devices, device);

    release_spinlock(&devfs_lock);

    return 0;
}

int devfs_inode_mode_todentry_type(mode_t inode_mode)
{
    int dirent_type;
    mode_t inode_type = inode_mode & INODE_TYPE_MASK;
    switch (inode_type)
    {
        case INODE_FLAG_CHARDEVICE:
            dirent_type = DT_CHR;
            break;
        case INODE_TYPE_DIRECTORY:
            dirent_type = DT_DIR;
            break;
        default:
            dirent_type = DT_CHR;
            break;
    }

    return dirent_type;
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

    int dirent_type = devfs_inode_mode_todentry_type(device->inode->mode);

    struct dirent* result = new_vfs_dirent();
    result->inode = device->inode->inode;
    result->offset = index;
    result->type = dirent_type;
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
    struct devfs_device* device = NULL;

    for (size_t i = 0; i < devfs_devices->size; i++) {
        
        device = (struct devfs_device*)current->element;

        if (device->inode->inode == ino_num) {
            result = device->inode;
            goto exit;
        }

        current = current->next;
    }

exit:
    release_spinlock(&devfs_lock);
    return result;
}

uint64 devfs_find_dentry(struct superblock* sb, struct inode* parent_inode, const char *name, int* type)
{
    if (parent_inode->inode != DEVFS_ROOT_INODE) {
        return WRONG_INODE_INDEX;
    }

    uint64_t inode = WRONG_INODE_INDEX;

    struct devfs_device* dev = devfs_find_dev(name);
    if (dev != NULL) {
        inode = dev->inode->inode;
        *type = devfs_inode_mode_todentry_type(dev->inode->mode);
    }

    return inode;
}