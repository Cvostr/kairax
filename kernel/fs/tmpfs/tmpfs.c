#include "tmpfs.h"
#include "mem/kheap.h"
#include "string.h"

struct file_operations tmpfs_file_ops = {
    .read = tmpfs_file_read,
    .write = tmpfs_file_write
};
struct file_operations tmpfs_dir_ops = {
    .readdir = tmpfs_file_readdir
};

struct inode_operations tmpfs_file_inode_ops;
struct inode_operations tmpfs_dir_inode_ops = {
    .mkfile = tmpfs_mkfile,
    .mkdir = tmpfs_mkdir,
    .unlink = tmpfs_unlink,
    .rmdir = tmpfs_rmdir
};

struct super_operations tmpfs_sb_ops = {
    .read_inode = tmpfs_read_node,
    .find_dentry = tmpfs_find_dentry,
    .destroy_inode = tmpfs_purge_inode
};

void tmpfs_init()
{
    filesystem_t* tmpfs = new_filesystem();
    tmpfs->name = "tmpfs";
    tmpfs->mount = tmpfs_mount;
    tmpfs->unmount = tmpfs_unmount;

    filesystem_register(tmpfs);
}

#define INODES_ADDITIVE 3
ino_t tmpfs_instance_add_inode(struct tmpfs_instance *inst, struct tmpfs_inode* inode, ino_t index)
{
    if (index == 0)
    {
        // Сначала ищем свободный номер
        for (ino_t i = 2; i < inst->inodes_allocated; i ++)
        {
            if (inst->inodes[i] == NULL)
            {
                index = i;
                break;
            }
        }

        // Все номера заняты
        if (index == 0)
        {
            // Используем следующий номер после всех выделенных
            index = inst->inodes_allocated;
        }
    }

    // свободных нет - расширяем массив указателей
    size_t new_size = (index + 1 + INODES_ADDITIVE);
    size_t new_size_bytes = new_size * sizeof(struct tmpfs_inode*);
    if (new_size > inst->inodes_allocated)
    {
        struct tmpfs_inode **inodes = kmalloc(new_size_bytes);
        memset(inodes, 0, new_size_bytes);
        memcpy(inodes, inst->inodes, inst->inodes_allocated * sizeof(struct tmpfs_inode*));
        if (inst->inodes)
            kfree(inst->inodes);
        inst->inodes = inodes;

        inst->inodes_allocated = index + 1 + INODES_ADDITIVE;
    }

    inst->inodes[index] = inode;
    atomic_inc(&inode->refs);

    return index;
}

void tmpfs_instance_remove_inode(struct tmpfs_instance *inst, ino_t index)
{
    if (index >= inst->inodes_allocated)
    {
        return NULL;
    }

    struct tmpfs_inode *node = inst->inodes[index]; 

    if (node != NULL)
    {
        tmpfs_free_inode(node);
        inst->inodes[index] = NULL;
    }
}

struct inode* tmpfs_inode_to_vfs_inode(struct tmpfs_instance *inst, struct tmpfs_inode* inode, ino_t ino_num)
{
    struct inode* result = new_vfs_inode();
    result->inode = ino_num;
    result->sb = inst->vfs_sb;
    result->uid = inode->uid;
    result->gid = inode->gid;
    result->size = inode->size;
    //result->blocks = inode->num_blocks;
    result->mode = inode->mode;
    //result->access_time.tv_sec = inode->atime;
    //result->create_time.tv_sec = inode->ctime;
    //result->modify_time.tv_sec = inode->mtime;
    result->hard_links = inode->hard_links;
    result->device = (dev_t) inst;

    if ((inode->mode & INODE_TYPE_MASK) == INODE_TYPE_FILE) {
        result->operations = &tmpfs_file_inode_ops;
        result->file_ops = &tmpfs_file_ops;
    }

    if ((inode->mode & INODE_TYPE_MASK) == INODE_TYPE_DIRECTORY) {
        result->operations = &tmpfs_dir_inode_ops;
        result->file_ops = &tmpfs_dir_ops;
    }
    if ((inode->mode & INODE_TYPE_MASK) == INODE_FLAG_SYMLINK) {
        //result->operations = &symlink_inode_ops;
    }
    if ((inode->mode & INODE_TYPE_MASK) == INODE_FLAG_BLOCKDEVICE) {
    
    }

    return result;
}

struct tmpfs_inode* tmpfs_get_inode(struct tmpfs_instance *inst, ino_t inode)
{
    if (inode >= inst->inodes_allocated)
    {
        return NULL;
    }

    struct tmpfs_inode *node = inst->inodes[inode]; 

    if (node != NULL)
        atomic_inc(&node->refs);

    return node;
}

struct tmpfs_dentry* tmpfs_get_dentry(struct tmpfs_inode *inode, const char *name)
{
    for (size_t i = 0; i < inode->dentries_allocated; i ++)
    {
        struct tmpfs_dentry *dentry = inode->dentries[i];
        if (dentry != NULL)
        {
            if (strcmp(dentry->name, name) == 0)
            {
                return dentry;
            }
        }
    }

    return NULL;
}

struct tmpfs_dentry* tmpfs_make_dentry(uint64_t inode, const char *name, int type)
{
    size_t bufsize = sizeof(struct tmpfs_dentry) + strlen(name) + 1;

    struct tmpfs_dentry *dentry = kmalloc(bufsize);
    memset(dentry, 0, bufsize);

    dentry->inode = inode;
    dentry->type = type;
    strcpy(dentry->name, name);

    return dentry;
}

#define DENTRIES_ADDITIVE 3
void tmpfs_inode_add_dentry(struct tmpfs_inode *inode, struct tmpfs_dentry *dentry)
{
    for (size_t i = 0; i < inode->dentries_allocated; i ++)
    {
        if (inode->dentries[i] == NULL)
        {
            inode->dentries[i] = dentry;
            return;
        }
    }

    size_t old_array_sz = (inode->dentries_allocated) * sizeof(struct tmpfs_dentry *);
    size_t array_sz = (inode->dentries_allocated + DENTRIES_ADDITIVE) * sizeof(struct tmpfs_dentry *);
    
    struct tmpfs_dentry **array = kmalloc(array_sz);
    memset(array, 0, array_sz);
    memcpy(array, inode->dentries, old_array_sz);
    array[inode->dentries_allocated] = dentry;

    if (inode->dentries)
        kfree(inode->dentries);
    inode->dentries = array;
    inode->dentries_allocated += DENTRIES_ADDITIVE;
}

void tmpfs_inode_remove_dentry(struct tmpfs_inode *inode, struct tmpfs_dentry *dentry)
{
    for (size_t i = 0; i < inode->dentries_allocated; i ++)
    {
        if (inode->dentries[i] == dentry)
        {
            kfree(inode->dentries[i]);
            inode->dentries[i] = NULL;
        }
    }
}

struct inode* tmpfs_mount(drive_partition_t* drive, struct superblock* sb)
{
    struct tmpfs_instance *inst = kmalloc(sizeof(struct tmpfs_instance));
    memset(inst, 0, sizeof(struct tmpfs_instance));
    inst->vfs_sb = sb;
    inst->blocksize = 4096;

    struct tmpfs_inode *root_inode = kmalloc(sizeof(struct tmpfs_inode));
    memset(root_inode, 0, sizeof(struct tmpfs_inode));
    root_inode->uid = 0;
    root_inode->gid = 0;
    root_inode->mode = INODE_TYPE_DIRECTORY | 0777;

    tmpfs_instance_add_inode(inst, root_inode, 2);

    sb->fs_info = inst;
    sb->blocksize = inst->blocksize;
    sb->operations = &tmpfs_sb_ops;

    return tmpfs_inode_to_vfs_inode(inst, root_inode, 2);
}

void tmpfs_free_inode(struct tmpfs_inode* inode)
{
    if (atomic_dec_and_test(&inode->refs))
    {
        for (size_t j = 0; j < inode->dentries_allocated; j ++)
        {
            struct tmpfs_dentry *dentry = inode->dentries[j];
            if (dentry != NULL)
            {
                kfree(dentry);
            }
        }

        // TODO: освободить данные файла

        kfree(inode);
    }
}

int tmpfs_unmount(struct superblock* sb)
{
    struct tmpfs_instance *inst = (struct tmpfs_instance*) sb->fs_info;

    for (size_t i = 0; i < inst->inodes_allocated; i ++)
    {
        struct tmpfs_inode *inode = inst->inodes[i];
        if (inode != NULL)
        {
            tmpfs_free_inode(inode);
        }
    }

    // Удалить основную структуру
    kfree(inst);

    return 0;
}

struct inode* tmpfs_read_node(struct superblock* sb, uint64_t ino_num)
{
    struct tmpfs_instance *inst = (struct tmpfs_instance*) sb->fs_info;
    struct tmpfs_inode *inode = tmpfs_get_inode(inst, ino_num);
    if (inode == NULL)
        return NULL;

    struct inode *result = tmpfs_inode_to_vfs_inode(inst, inode, ino_num);

    tmpfs_free_inode(inode);

    return result;
}

uint64_t tmpfs_find_dentry(struct superblock* sb, struct inode* vfs_parent_inode, const char *name, int* type)
{
    uint64_t ino_num = -WRONG_INODE_INDEX;
    struct tmpfs_instance *inst = (struct tmpfs_instance*) sb->fs_info;
    // Получить inode - родителя
    struct tmpfs_inode *parent_inode = tmpfs_get_inode(inst, vfs_parent_inode->inode);
    if (parent_inode == NULL)
        return -WRONG_INODE_INDEX;

    struct tmpfs_dentry *dent = tmpfs_get_dentry(parent_inode, name);
    if (dent != NULL)
    {
        ino_num = dent->inode;
        *type = dent->type;
    }

    tmpfs_free_inode(parent_inode);
    return ino_num;
}

void tmpfs_purge_inode(struct inode* inode)
{
    struct tmpfs_instance *inst = (struct tmpfs_instance *) inode->sb->fs_info;
    tmpfs_instance_remove_inode(inst, inode->inode);
}

struct dirent* tmpfs_file_readdir(struct file* dir, uint32_t index)
{
    struct dirent* result = NULL;
    struct inode* vfs_inode = dir->inode;
    struct tmpfs_instance * inst = (struct tmpfs_instance *) vfs_inode->sb->fs_info;

    struct tmpfs_inode *inode = tmpfs_get_inode(inst, vfs_inode->inode);
    if (inode == NULL)
        goto exit;

    if (index >= inode->dentries_allocated)
        goto exit;

    struct tmpfs_dentry *dentry = inode->dentries[index];
    if (dentry == NULL)
        goto exit;

    result = new_vfs_dirent();
    result->inode = dentry->inode;
    result->type = dentry->type;
    strcpy(result->name, dentry->name);

exit:
    tmpfs_free_inode(inode);
    return result;
}

int tmpfs_mkfile(struct inode* parent, const char* file_name, uint32_t mode)
{
    int rc = 0; 
    struct tmpfs_instance *inst = (struct tmpfs_instance*) parent->sb->fs_info;

    // Получить inode - родителя
    struct tmpfs_inode *parent_inode = tmpfs_get_inode(inst, parent->inode);
    if (parent_inode == NULL)
        return -ENOENT;

    if (tmpfs_get_dentry(parent_inode, file_name) != NULL) {
        rc = -EEXIST;
        goto exit;
    }

    struct tmpfs_inode *new_inode = kmalloc(sizeof(struct tmpfs_inode));
    memset(new_inode, 0, sizeof(struct tmpfs_inode));
    new_inode->uid = 0;
    new_inode->gid = 0;
    new_inode->mode = INODE_TYPE_FILE | (0xFFF & mode);
    new_inode->hard_links = 1;

    // Добавляем inode в список
    ino_t ino_idx = tmpfs_instance_add_inode(inst, new_inode, 0);

    // Создаем dentry
    struct tmpfs_dentry *dentry = tmpfs_make_dentry(ino_idx, file_name, DT_REG);

    // Добавить dentry в директорию
    tmpfs_inode_add_dentry(parent_inode, dentry);

exit:
    tmpfs_free_inode(parent_inode);
    return rc;
}

int tmpfs_mkdir(struct inode* parent, const char* dir_name, uint32_t mode)
{
    int rc = 0; 
    struct tmpfs_instance *inst = (struct tmpfs_instance*) parent->sb->fs_info;

    // Получить inode - родителя
    struct tmpfs_inode *parent_inode = tmpfs_get_inode(inst, parent->inode);
    if (parent_inode == NULL)
        return -ENOENT;

    if (tmpfs_get_dentry(parent_inode, dir_name) != NULL) {
        rc = -EEXIST;
        goto exit;
    }

    struct tmpfs_inode *new_inode = kmalloc(sizeof(struct tmpfs_inode));
    memset(new_inode, 0, sizeof(struct tmpfs_inode));
    new_inode->uid = 0;
    new_inode->gid = 0;
    new_inode->mode = INODE_TYPE_DIRECTORY | (0xFFF & mode);
    new_inode->hard_links = 2;

    // Добавляем inode в список
    ino_t ino_idx = tmpfs_instance_add_inode(inst, new_inode, 0);

    // Создаем dentry
    struct tmpfs_dentry *dentry = tmpfs_make_dentry(ino_idx, dir_name, DT_DIR);

    // Добавить dentry в директорию
    tmpfs_inode_add_dentry(parent_inode, dentry);

exit:
    tmpfs_free_inode(parent_inode);
    return rc;
}

ssize_t tmpfs_file_read(struct file* file, char* buffer, size_t count, loff_t offset)
{

}

ssize_t tmpfs_file_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{

}

int tmpfs_unlink(struct inode* parent, struct dentry* dent)
{
    int rc = 0;
    struct tmpfs_instance *inst = (struct tmpfs_instance*) parent->sb->fs_info;

    struct tmpfs_inode *parent_inode = NULL;
    struct tmpfs_dentry *tmpfs_dent = NULL;
    struct tmpfs_inode *inode = NULL;

    // Получить родительскую Inode
    parent_inode = tmpfs_get_inode(inst, parent->inode);
    if (parent_inode == NULL) {
        rc = -ENOENT;
        goto exit;
    }

    // Получить удаляемую dentry
    tmpfs_dent = tmpfs_get_dentry(parent_inode, dent->name);
    if (dent == NULL) {
        rc = -ENOENT;
        goto exit;
    }

    // Получить удаляемую Inode
    inode = tmpfs_get_inode(inst, tmpfs_dent->inode);
    if (parent_inode == NULL) {
        rc = -ENOENT;
        goto exit;
    }

    // Удалить dentry
    tmpfs_inode_remove_dentry(parent_inode, tmpfs_dent);

    // Уменьшить количество ссылок на выбранную inode
    inode->hard_links--;

    // Уменьшить количество ссылок на выбранную inode в VFS
    dent->d_inode->hard_links--;

exit:
    if (parent_inode)
        tmpfs_free_inode(parent_inode);

    if (inode)
        tmpfs_free_inode(inode);

    return rc;
}

int tmpfs_rmdir(struct inode* parent, struct dentry* dentry)
{
    int rc = 0;

    struct tmpfs_inode *parent_inode = NULL;
    struct tmpfs_dentry *tmpfs_dent = NULL;
    struct tmpfs_inode *inode = NULL;

    struct tmpfs_instance *inst = (struct tmpfs_instance *) parent->sb->fs_info;

    if (dentry->d_inode->hard_links > 2) {
        rc = -ENOTEMPTY;
        goto exit;
    }

    // Получить родительскую Inode
    parent_inode = tmpfs_get_inode(inst, parent->inode);
    if (parent_inode == NULL) {
        rc = -ENOENT;
        goto exit;
    }

    // Получить удаляемую dentry
    tmpfs_dent = tmpfs_get_dentry(parent_inode, dentry->name);
    if (dentry == NULL) {
        rc = -ENOENT;
        goto exit;
    }

    // Получить удаляемую Inode
    inode = tmpfs_get_inode(inst, tmpfs_dent->inode);
    if (parent_inode == NULL) {
        rc = -ENOENT;
        goto exit;
    }

    // Проверить, что директория пуста
    for (size_t j = 0; j < inode->dentries_allocated; j ++)
    {
        if (inode->dentries[j] != NULL)
        {
            rc = -ENOTEMPTY;
            goto exit;
        }
    }

    // Удалить dentry
    tmpfs_inode_remove_dentry(parent_inode, tmpfs_dent);
    // Уменьшить количество ссылок на выбранную inode
    inode->hard_links = 0;
    // Уменьшить количество ссылок на выбранную inode в VFS
    dentry->d_inode->hard_links = 0;


    parent_inode->hard_links--;
    // Уменьшить количество ссылок на родительскую inode в VFS
    parent->hard_links --;

exit:
    if (parent_inode)
        tmpfs_free_inode(parent_inode);

    if (inode)
        tmpfs_free_inode(inode);

    return 0;
}