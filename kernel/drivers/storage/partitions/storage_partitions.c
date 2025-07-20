#include "storage_partitions.h"
#include "mem/kheap.h"
#include "list/list.h"
#include "mem/vmm.h"
#include "string.h"

#include "formats/mbr.h"
#include "formats/gpt.h"

#include "stdio.h"
#include "kstdlib.h"

list_t* partitions = NULL;

drive_partition_t* new_drive_partition_header()
{
    drive_partition_t* result = (drive_partition_t*)kmalloc(sizeof(drive_partition_t));
    memset(result, 0, sizeof(drive_partition_t));
    return result;
}

int add_partitions_from_device(struct device* device)
{
    char* first_sector = kmalloc(512);
    memset(first_sector, 0, 512);
    char* second_sector = kmalloc(512);
    memset(second_sector, 0, 512);

    int rc;

    rc = drive_device_read(device, 0, 1, first_sector);
    if (rc != 0)
    {
        printk("Error reading sector 0 of drive %s\n", device->dev_name);
        return rc;
    }

    rc = drive_device_read(device, 1, 1, second_sector);
    if (rc != 0)
    {
        printk("Error reading sector 1 of drive %s\n", device->dev_name);
        return rc;
    }

    mbr_header_t* mbr_header = (mbr_header_t*)first_sector;
    gpt_header_t* gpt_header = (gpt_header_t*)second_sector; 

    int has_mbr_sign = mbr_header->signature == MBR_CHECK_SIGNATURE;
    int has_gpt_sign = check_gpt_header_signature(gpt_header);

    if (has_gpt_sign == FALSE && has_mbr_sign == FALSE)
    {
        printk("Disk %s does not have partition table\n", device->dev_name);
        return 0;
    }

    if (has_mbr_sign)
    {
        //Это MBR разметка
        for (uint32_t i = 0; i < 4; i ++) {
            mbr_partition_t* mbr_partition = &mbr_header->partition[i];

            // Заголовок MBR также содержится если раздел формата GPT
            if (mbr_partition->lba_sectors == 0 || mbr_partition->type == MBR_PARTITION_TYPE_GPT) {
                continue;
            }

            drive_partition_t* partition = new_drive_partition_header();
            partition->device = device;
            partition->index = i;
            partition->start_lba = mbr_partition->lba_start;
            partition->sectors = mbr_partition->lba_sectors;

            strcpy(partition->name, device->drive_info->blockdev_name);
			strcat(partition->name, "p");
			strcat(partition->name, itoa(i, 10));

            add_partition_header(partition);
        }
    }

    if (has_mbr_sign && has_gpt_sign) 
    {
        //Это GPT разметка
        uint32_t entries_num = gpt_header->gpea_entries_num;
        uint32_t current_lba = gpt_header->gpea_lba;

        uint32_t found_partitions = 0;
        for (int i = 0; i < entries_num; i ++)
        {
            char* gpt_entry_buffer = kmalloc(GPT_BLOCK_SIZE);
            memset(gpt_entry_buffer, 0, GPT_BLOCK_SIZE);
            
            rc = drive_device_read(device, current_lba, 1, gpt_entry_buffer);
            if (rc != 0)
            {
                printk("Error reading sector %i of drive %s\n", current_lba, device->dev_name);
                return rc;
            }

            for (int offset = 0; offset < GPT_BLOCK_SIZE; offset += gpt_header->gpea_entry_size)
            {
                gpt_entry_t* entry = (gpt_entry_t*)(gpt_entry_buffer + offset);

                if (guid_is_zero(&entry->partition_guid)){
                    entries_num = 0;
                    break;
                }

                //Формирование структуры, описывающей раздел
                drive_partition_t* partition = new_drive_partition_header();
                partition->device = device;
                partition->index = found_partitions;
                partition->start_lba = entry->start_lba;
                partition->sectors = (entry->end_lba - entry->start_lba) + 1;

                //Формирование имени раздела
                strcpy(partition->name, device->drive_info->blockdev_name);
                strcat(partition->name, "p");
                strcat(partition->name, itoa(found_partitions, 10));

                add_partition_header(partition);
                found_partitions++;
            }

            kfree(gpt_entry_buffer);

            current_lba++;
        }
    }

    kfree(first_sector);
    kfree(second_sector);

    return 0;
}

void add_partition_header(drive_partition_t* partition_header)
{
    if (partitions == NULL)
        partitions = create_list();

    list_add(partitions, partition_header);
}

uint32_t get_partitions_count()
{
    if (partitions == NULL)
        return 0;
    return partitions->size;
}

drive_partition_t* get_partition(uint32_t index)
{
    return (drive_partition_t*)list_get(partitions, index);
}

drive_partition_t* get_partition_with_name(const char* name)
{
    for (uint32_t i = 0; i < partitions->size; i ++) {
        drive_partition_t* partition = (drive_partition_t*)list_get(partitions, i);
        if (strcmp(partition->name, name) == 0)
            return partition;
    }
    return NULL;
}

int partition_read(drive_partition_t* partition, uint64_t lba_start, uint64_t count, char* buffer)
{
    return drive_device_read(partition->device, lba_start + partition->start_lba, count, buffer);
}

int partition_write(drive_partition_t* partition, uint64_t lba_start, uint64_t count, char* buffer)
{
    return drive_device_write(partition->device, lba_start + partition->start_lba, count, buffer);
}