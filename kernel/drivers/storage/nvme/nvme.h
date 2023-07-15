#ifndef _NVME_H
#define _NVME_H

#include "bus/pci/pci.h"
#include "types.h"

#define NVME_CMD_IO_READ    0x02
#define NVME_CMD_IO_WRITE   0x01
#define NVME_CMD_IDENTIFY   0x06

struct nvme_bar0 {
    uint64_t    cap;
    uint32_t    version;
    uint32_t    int_mask_set;
    uint32_t    int_mask_clear;
    uint32_t    conf;
    uint32_t    unused1;
    uint32_t    status;
    uint32_t    unused2;

    uint32_t    a_queue_attrs;
    uint64_t    a_submit_queue;
    uint64_t    a_compl_queue;
} PACKED;

struct nvme_cmd_rw {
    uint64_t    start_lba;
    uint16_t    block_count;
} PACKED;

struct nvme_cmd_id {
    uint32_t    id_type;    //  0 - namespace, 1 - controller, 2 - namespace list.
} PACKED;

struct nvme_command {
    uint32_t    cmd;
    uint32_t    namespace_id;
    uint32_t    reserved[2];
    uint64_t    metadata_ptr;
    uint64_t    prp[2];
} PACKED;

struct nvme_device {
    pci_device_desc*    pci_device;
    struct nvme_bar0*          bar0;
};

void init_nvme();

#endif