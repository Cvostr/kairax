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

struct inode* superblock_get_inode(struct superblock* sb, uint64 inode)
{
    struct list_node* current = sb->inodes->head;
    struct inode* node = (struct inode*)current->element;

    for(unsigned int i = 0; i < sb->inodes->size; i++) {
        if (node->inode == inode)
            return node;
            
        current = current->next;
        node = (struct inode*)current->element;
    }

    return NULL;
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