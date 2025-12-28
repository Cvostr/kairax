#include "net_device.h"
#include "mem/kheap.h"
#include "kairax/list/list.h"
#include "kairax/string.h"
#include "kairax/errors.h"
#include "kairax/stdio.h"

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
    int i = 0;
    char namebuf[NIC_NAME_LEN];

    acquire_spinlock(&nic_lock);
    
    for (;;) 
    {
        sprintf(namebuf, sizeof(namebuf), "%s%i", name_prefix, i++);

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

int nic_set_addr_ip4(struct nic* nic, uint32_t addr)
{
    if (nic->flags & NIC_FLAG_LOOPBACK) {
        return -ERROR_WRONG_FUNCTION;
    }

    nic->ipv4_addr = addr;

    return 0;
}

int nic_set_subnetmask_ip4(struct nic* nic, uint32_t subnet)
{
    if (nic->flags & NIC_FLAG_LOOPBACK) {
        return -ERROR_WRONG_FUNCTION;
    }

    nic->ipv4_subnet = subnet;

    return 0;
}

int nic_set_flags(struct nic* nic, uint32_t flags)
{
    if (nic->flags & NIC_FLAG_LOOPBACK) {
        return -ERROR_WRONG_FUNCTION;
    }

    if ((nic->flags & NIC_FLAG_UP) != (flags & NIC_FLAG_UP)) {
        // Бит UP флагов сменился
        if ((flags & NIC_FLAG_UP) == NIC_FLAG_UP) {
            nic->up(nic);
        } else {
            nic->down(nic);
        }
    }

    nic->flags = flags;
}