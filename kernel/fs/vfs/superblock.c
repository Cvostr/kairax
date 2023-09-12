#include "superblock.h"
#include "mem/kheap.h"
#include "string.h"

struct superblock* new_superblock()
{
    struct superblock* result = (struct superblock*)kmalloc(sizeof(struct superblock));
    memset(result, 0, sizeof(struct superblock));
    result->inodes = create_list();
    return result;
}

void free_superblock(struct superblock* sb)
{
    free_list(sb->inodes);
    kfree(sb);
}

struct inode* superblock_get_cached_inode(struct superblock* sb, uint64 inode)
{
    acquire_spinlock(&sb->spinlock);
    struct list_node* current = sb->inodes->head;
    struct inode* node = NULL;
    struct inode* result = NULL;

    for (size_t i = 0; i < sb->inodes->size; i++) {

        node = (struct inode*)current->element;

        if (node->inode == inode) {
            result = node;
            goto exit;
        }
            
        // Переход на следующий элемент
        current = current->next;
    }

exit:
    release_spinlock(&sb->spinlock);
    
    return result;
}

struct inode* superblock_get_inode(struct superblock* sb, uint64 inode)
{
    // Попробовать найти inode в кэше
    struct inode* result = superblock_get_cached_inode(sb, inode);
    if (result) {
        return result;
    }
    
    acquire_spinlock(&sb->spinlock);
    // Не нашли, пробуем считать с диска
    result = sb->operations->read_inode(sb, inode);
    release_spinlock(&sb->spinlock);
    return result;
}

struct dentry* superblock_get_dentry(struct superblock* sb, struct dentry* parent, const char* name)
{
    struct dentry* result = NULL;
    acquire_spinlock(&sb->spinlock);

    result = dentry_get_child_with_name(parent, name);
    if (result != NULL)
        goto exit;
        
    // считать dentry с диска
    uint64_t inode = sb->operations->find_dentry(sb, parent->inode, name);
    if (inode != WRONG_INODE_INDEX) {
        // нашелся объект с указанным именем
        result = new_dentry();
        strcpy(result->name, name);
        result->parent = parent;
        result->sb = sb;
        result->inode = inode;
        // Добавить в список подпапок родителя
        dentry_add_subdir(parent, result); 
    }
exit:

    release_spinlock(&sb->spinlock);
    return result;
}

void superblock_add_inode(struct superblock* sb, struct inode* inode)
{
    acquire_spinlock(&sb->spinlock);

    list_add(sb->inodes, inode);

    release_spinlock(&sb->spinlock);
}

void superblock_remove_inode(struct superblock* sb, struct inode* inode)
{
    acquire_spinlock(&sb->spinlock);

    list_remove(sb->inodes, inode);

    release_spinlock(&sb->spinlock);
}