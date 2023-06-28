#include "dentry.h"
#include "mem/kheap.h"
#include "string.h"

struct dentry* new_dentry()
{
    struct dentry* result = kheap(sizeof(struct dentry));
    memset(result, 0, sizeof(struct dentry));
    result->subdirs = create_list();
    return result;
}

void free_dentry(struct dentry* dentry)
{
    free_list(dentry->subdirs);
    kfree(dentry);
}

struct dentry* dentry_get_child_with_name(struct dentry* parent, const char* child_name)
{
    if(parent == NULL)
        return NULL;

    struct list_node* current = parent->subdirs->head;
    struct dentry* child = (struct dentry*)current->element;

    for(unsigned int i = 0; i < parent->subdirs->size; i++){
        if (strcmp(child->name, child_name) == 0)
            return child;
            
        current = current->next;
    }
    
    return NULL;
}