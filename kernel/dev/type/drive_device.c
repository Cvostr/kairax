#include "drive_device.h"
#include "mem/kheap.h"
#include "string.h"
#include "kairax/list/list.h"
#include "drivers/storage/partitions/storage_partitions.h"

list_t drive_list = {0,};
spinlock_t drive_list_lock = 0;

struct drive_device_info* new_drive_device_info()
{
    struct drive_device_info* result = kmalloc(sizeof(struct drive_device_info));
    memset(result, 0, sizeof(struct drive_device_info));
    return result;
}

int drive_device_read( struct drive_device_info* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            char* buffer)
{
    return drive->read(drive->dev, start_lba, count, buffer);
}

int drive_device_write(struct drive_device_info* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            const char* buffer)
{
    return drive->write(drive->dev, start_lba, count, buffer);
}

struct drive_device_info* get_drive_by_blockname_unlocked(const char* name)
{
    struct list_node* current_node = drive_list.head;
    struct drive_device_info* drive = NULL;

    while (current_node != NULL) {
        
        drive = (struct drive_device_info*) current_node->element;

        if (strcmp(drive->blockdev_name, name) == 0) {
            return drive;
        }
        
        // Переход на следующий элемент
        current_node = current_node->next;
    }

    return NULL;
}

int register_drive(struct drive_device_info* drive)
{
    int i = 0;
    struct drive_device_info* found = NULL;
    char namebuf[BLOCKDEV_NAMELEN + 1];

    char *placeholder = strchr(drive->blockdev_name, 'X');

    acquire_spinlock(&drive_list_lock);

    if (placeholder == NULL)
    {
        found = get_drive_by_blockname_unlocked(drive->blockdev_name);
        if (found == NULL)
        {
            list_add(&drive_list, drive);
            goto exit;
        }
        else
        {
            release_spinlock(&drive_list_lock);
            return -EEXIST;
        }
    }

    *placeholder = '\0';
    
    for (;;) 
    {
        sprintf(namebuf, sizeof(namebuf), "%s%i", drive->blockdev_name, i++);
        
        found = get_drive_by_blockname_unlocked(namebuf);

        if (found != NULL) {
            continue;
        } else {
            strncpy(drive->blockdev_name, namebuf, BLOCKDEV_NAMELEN);
            break;
        }
    }

    list_add(&drive_list, drive);

exit:
    release_spinlock(&drive_list_lock);

    // Добавим все разделы с этого диска
    add_partitions_from_drive(drive);
    return 0;
}