#include "net_device.h"
#include "mem/kheap.h"
#include "kairax/list/list.h"
#include "kairax/string.h"

list_t nic_list = {0,};
spinlock_t nic_lock = 0;

struct nic* new_nic()
{
    struct nic* nic = kmalloc(sizeof(struct nic));
    memset(nic, 0, sizeof(struct nic));
    return nic;
}

struct nic* get_nic(int index)
{
    acquire_spinlock(&nic_lock);
    struct nic* result = list_get(&nic_list, index);
    release_spinlock(&nic_lock);

    return result;
}

struct nic* get_nic_by_name_unlocked(const char* name)
{
    struct list_node* current_node = nic_list.head;
    struct nic* nic = NULL;

    while (current_node != NULL) {
        
        nic = (struct nic*) current_node->element;

        if (strcmp(nic->name, name) == 0) {
            return nic;
        }
        
        // Переход на следующий элемент
        current_node = current_node->next;
    }

    return NULL;
}

struct nic* get_nic_by_name(const char* name)
{
    acquire_spinlock(&nic_lock);
    struct nic* result = get_nic_by_name_unlocked(name);
    release_spinlock(&nic_lock);

    return result;
}

int register_nic(struct nic* nic, const char* name_prefix)
{
    // TODO: двузначные значения постфиксов
    char postfix[] = {0, 0};
    int i = 0;
    char namebuf[NIC_NAME_LEN];

    acquire_spinlock(&nic_lock);
    
    for (int i = 0; i < 9; i ++) {
        strcpy(namebuf, name_prefix);
        postfix[0] = i + 0x30;
        strcat(namebuf, postfix);

        struct nic* n = get_nic_by_name_unlocked(namebuf);
        if (n != NULL) {
            continue;
        } else {
            strcpy(nic->name, namebuf);
            break;
        }
    }

    list_add(&nic_list, nic);
exit:
    release_spinlock(&nic_lock);
    return 0;
}