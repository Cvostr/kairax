#include "storage_partitions.h"
#include "mem/kheap.h"
#include "raxlib/list/list.h"
#include "string.h"

#include "formats/mbr.h"
#include "formats/gpt.h"

#include "stdio.h"

list_t* partitions = NULL;

drive_partition_header_t* new_drive_partition_header(){
    drive_partition_header_t* result = (drive_partition_header_t*)kmalloc(sizeof(drive_partition_header_t));
    memset(result, 0, sizeof(drive_partition_header_t));
    return result;
}

void add_partitions_from_device(drive_device_header_t* device){
    char first_sector[512];
    char second_sector[512];

    drive_device_read(device, 0, 0, 1, V2P(first_sector));
    drive_device_read(device, 1, 0, 1, V2P(second_sector));
    
    mbr_header_t* mbr_header = (mbr_header_t*)first_sector;
    gpt_header_t* gpt_header = (gpt_header_t*)second_sector; 

    int has_mbr_sign = mbr_header->signature == MBR_CHECK_SIGNATURE;
    int has_gpt_sign = check_gpt_header_signature(gpt_header);

    if(has_mbr_sign){
        //Это MBR разметка
        for(uint32_t i = 0; i < 4; i ++){
            mbr_partition_t* mbr_partition = &mbr_header->partition[i];

            if(mbr_partition->lba_sectors == 0 || mbr_partition->type == MBR_PARTITION_TYPE_GPT)
                continue;
            drive_partition_header_t* partition = new_drive_partition_header();
            partition->device = device;
            partition->index = i;
            partition->start_sector = mbr_partition->lba_start;
            partition->sectors = mbr_partition->lba_sectors;

            strcpy(partition->name, device->name);
			strcat(partition->name, "p");
			strcat(partition->name, itoa(i, 10));

            add_partition_header(partition);
        }
    }
    if(has_mbr_sign && has_gpt_sign){
        int entries_num = gpt_header->gpea_entries_num;
        uint32_t current_lba = gpt_header->gpea_lba;

        uint32_t found_partitions = 0;
        for(int i = 0; i < entries_num; i ++){
            char entry_buffer[GPT_BLOCK_SIZE];
            drive_device_read1(device, current_lba, 1, V2P(entry_buffer));
            for(int offset = 0; offset < GPT_BLOCK_SIZE; offset += gpt_header->gpea_entry_size){
                gpt_entry_t* entry = (gpt_entry_t*)(entry_buffer + offset);

                if(guid_is_zero(&entry->partition_guid)){
                    entries_num = 0;
                    break;
                }

                drive_partition_header_t* partition = new_drive_partition_header();
                partition->device = device;
                partition->index = found_partitions;
                partition->start_sector = entry->start_lba;
                partition->sectors = (entry->end_lba - entry->start_lba) + 1;

                strcpy(partition->name, device->name);
                strcat(partition->name, "p");
                strcat(partition->name, itoa(found_partitions, 10));

                add_partition_header(partition);
                found_partitions++;
            }
            current_lba++;
        }
    }

}

void add_partition_header(drive_partition_header_t* partition_header){
    if(partitions == NULL)
        partitions = create_list();

    list_add(partitions, partition_header);
}

uint32_t get_partitions_count(){
    if(partitions == NULL)
        return 0;
    return partitions->size;
}

drive_partition_header_t* get_partition(uint32_t index){
    return (drive_partition_header_t*)list_get(partitions, index);
}

drive_partition_header_t* get_partition_with_name(char* name){
    for(uint32_t i = 0; i < partitions->size; i ++){
        drive_partition_header_t* partition = (drive_partition_header_t*)list_get(partitions, i);
        if(strcmp(partition->name, name) == 0)
            return partition;
    }
    return NULL;
}