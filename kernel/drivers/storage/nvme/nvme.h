#ifndef _NVME_H
#define _NVME_H

#include "dev/bus/pci/pci.h"
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

    uint32_t    aqa;
    
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

    union {
        struct nvme_cmd_rw rwcmd;
        struct nvme_cmd_id idcmd;
    };
} PACKED;

struct nvme_compl_entry {
    uint32_t res;
    uint32_t reserved;
    uint16_t subm_queue_head;
    uint16_t subm_queue_id;
    uint16_t cmd_id;
    uint16_t status;
} PACKED;

struct nvme_lbaf {
    uint16_t    metadata_size;
    uint8_t     lba_data_size;
    uint8_t     rel_perf;
} PACKED;

struct nvme_namespace_id {
    uint64_t    ns_size;
    uint64_t    ns_capacity;
    uint64_t    ns_utilization; //NUSE
    uint8_t     ns_features;
    uint8_t     ns_nlbaf;       //Number of LBA formats
    uint8_t     ns_flbas;       // Formatted LBA size
    uint8_t     ns_mc;          // metadata capabilities
    uint8_t     ns_dpc;         // Data protection capabilities
    uint8_t     ns_dps;
    uint8_t     ns_mic;
    uint8_t     ns_rescap;
    uint8_t     ns_fpi;
    uint8_t     ns_dlfeat;
    uint16_t    ns_awun;
    uint16_t    ns_awupf;
    uint16_t    ns_acwu;
    uint16_t    ns_absn;
    uint16_t    ns_abo;
    uint16_t    ns_abspf;
    uint16_t    ns_oiob;
    uint64_t    ns_nvmcap[2];
    uint16_t    ns_pwg;
    uint16_t    ns_pwa;
    uint16_t    ns_pdg;
    uint16_t    ns_pda;
    uint16_t    ns_ows;
    uint16_t    ns_mssrl;
    uint16_t    ns_mcl;
    uint8_t     ns_msrc;
    uint32_t    ns_anagrpid;
    uint16_t    reserved;
    uint8_t     ns_attributes;
    uint16_t    ns_nvmsetid;
    uint16_t    ns_endgid;
    uint8_t     ns_guid[16];
    uint8_t     ns_eui64[8];
    struct nvme_lbaf lbafs[64];
    // TODO: Add vendor specific
};

struct nvme_controller_id {
    uint16_t    vid;
    uint16_t    ssvid;
    char        serial[20];
    char        model[40];
    char        fw_revision[8];
    uint8_t     rab;
    uint8_t     ieee[3];
    uint8_t     cmic;
    uint8_t     mdts;
    uint16_t    controller_id;
    uint32_t    version;
    uint32_t    rtd3r;
    uint32_t    rtd3e;
    uint32_t    oaes;
    uint32_t    ctratt;
    uint16_t    rrls;
    uint8_t     reserved[9];
    uint8_t     controller_type;
    uint8_t     fguid[16];
    uint16_t    crdt1;
    uint16_t    crdt2;
    uint16_t    crdt3;
    uint8_t     reserved2[106];
    uint8_t     reserved3[13];
    uint8_t     nvmsr;
    uint8_t     vpd_wci;
    uint8_t     mec;

    uint16_t    oacs;
    uint8_t     acl;    
    uint8_t     aerl;
    uint8_t     firmware;
    uint8_t     lpa;
    uint8_t     elpe;
    uint8_t     npss;
    uint8_t     avscc;
    uint8_t     apsta;
    uint16_t    wctemp;
    uint16_t    cctemp;

    uint16_t    mtfa;
    uint32_t    hmpre;
    uint32_t    hmmin;
    uint64_t    tnvmcap[2];
    uint64_t    unvmcap[2];
    uint32_t    rpmbs;
    uint16_t    edstt;
    uint8_t     dsto;
    uint8_t     fwug;
    uint16_t    kas;
    uint16_t    hctma;
    uint16_t    mntmt;
    uint16_t    mxtmt;
    uint32_t    sanicap;
    uint32_t    hmminds;
    uint16_t    hmmaxd;
    uint16_t    nsetidmax;
    uint16_t    endgidmax;
    uint8_t     anatt;
    uint8_t     anacap;
    uint32_t    anagrpmax;
    uint32_t    nanagripd;
    uint32_t    pels;
    uint16_t    domain_id;
    uint8_t     reserved4[10];
    uint64_t    megcap[2];
    uint8_t     reserved5[128];

    uint8_t     sqes;
    uint8_t     cqes;
    uint16_t    maxcmd;
    uint32_t    namespaces_num;
    uint16_t    oncs;
    uint16_t    fuses;
    uint8_t     fna;
    uint8_t     vwc;
    uint16_t    awun;
    uint16_t    awupf;
    uint8_t     icsvscc;
    uint8_t     nwpc;
    uint16_t    acwu;
    uint16_t    copy_desc_format;
    uint32_t    sgls;

    uint32_t unused6[1401];
    uint8_t vs[1024];
};

struct nvme_queue {
    volatile struct nvme_command *submit;
    volatile struct nvme_compl_entry *completion;
};

struct nvme_device {
    struct pci_device_info*    pci_device;
    struct nvme_bar0*   bar0;
    size_t              stride;
    size_t              queues_num;

    struct nvme_queue* admin_queue;
};

struct nvme_queue* nvme_create_queue(size_t slots);

void init_nvme();

#endif