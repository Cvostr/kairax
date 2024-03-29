#ifndef _GPT_H
#define _GPT_H

#include "types.h"
#include "guid/guid.h"

#define GPT_BLOCK_SIZE      512
#define GPT_SIGNATURE       "EFI PART"

typedef struct PACKED {
    char        signature[8];
    uint32_t    revision;
    uint32_t    header_size;
    uint32_t    header_crc32;
    uint32_t    rsv;
    uint64_t    header_lba;
    uint64_t    backup_header_lba;
    uint64_t    first_u_block;
    uint64_t    last_u_block;
    guid_t      disk_id;
    uint64_t    gpea_lba;
    uint32_t    gpea_entries_num;
    uint32_t    gpea_entry_size;
    uint32_t    gpea_crc32;

    char        rsv1[GPT_BLOCK_SIZE - 0x5C];
} gpt_header_t;

typedef struct PACKED {
    guid_t      type_guid;
    guid_t      partition_guid;
    uint64_t    start_lba;
    uint64_t    end_lba;
    uint64_t    attrs;
    char        name[];
} gpt_entry_t;

int check_gpt_header_signature(gpt_header_t* header);

#endif