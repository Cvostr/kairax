#ifndef _MBR_H
#define _MBR_H

#include "stdint.h"

#define MBR_PARTITION_TYPE_NULL 0x00
#define MBR_PARTITION_TYPE_GPT 0xEE
#define MBR_PARTITION_TYPE_EFI 0xEF

#define MBR_CHECK_SIGNATURE     0xAA55

typedef struct PACKED {
    uint32_t    attr : 8;
    uint32_t    chs_start : 24;
    uint32_t    type : 8;
    uint32_t    chs_end   : 24;
    uint32_t    lba_start;
    uint32_t    lba_sectors;
} mbr_partition_t;

typedef struct PACKED {
    char                bootblock[440];
    uint32_t            disk_id;
    uint16_t            rsv;
    mbr_partition_t     partition[4];
    uint16_t            signature;
} mbr_header_t;

#endif