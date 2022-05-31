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

    drive_device_read(device, 0, 0, 1, V2P(first_sector));
    
    mbr_header_t* mbr_header = (mbr_header_t*)first_sector;
    if(mbr_header->signature == MBR_CHECK_SIGNATURE){
        //Это MBR разметка
        for(uint32_t i = 0; i < 4; i ++){
            mbr_partition_t* mbr_partition = &mbr_header->partition[i];
            if(mbr_partition->lba_sectors == 0)
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
    }else{
        gpt_header_t* gpt_header = (gpt_header_t*)first_sector; 
        if(check_gpt_header_signature(gpt_header))
            printf("GPT PARTITION TABLE");
    }

}

void add_partition_header(drive_partition_header_t* partition_header){
    if(partitions == NULL)
        partitions = create_list();

    list_add(partitions, partition_header);
}

uint32_t get_partitions_count(){
    return partitions->size;
}

drive_partition_header_t* get_partition(uint32_t index){
    return (drive_partition_header_t*)list_get(partitions, index);
}