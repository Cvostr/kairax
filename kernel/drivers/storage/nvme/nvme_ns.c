#include "nvme.h"
#include "mem/kheap.h"
#include "kairax/string.h"

struct nvme_namespace* nvme_namespace(struct nvme_controller* controller, uint32_t id, struct nvme_namespace_id* nsid)
{
    struct nvme_namespace* ns = kmalloc(sizeof(struct nvme_namespace));
    memset(ns, 0, sizeof(struct nvme_namespace));

    ns->controller = controller;
    ns->namespace_id = id;

    uint8_t lba_fmt = nsid->ns_flbas & 0xf;
    ns->lba_size = nsid->lbafs[lba_fmt].lba_data_size;
    ns->block_size = 1 << ns->lba_size;

    ns->disk_size = nsid->ns_size;

    printk("NVME: Namespace %i: lba_sz %i blk_sz %i size %i\n", id, ns->lba_size, ns->block_size, ns->disk_size);

    return ns;
}

int nvme_namespace_read(struct nvme_namespace* ns, uint64_t lba, uint64_t count, unsigned char* out)
{

}

int nvme_namespace_write(struct nvme_namespace* ns, uint64_t lba, uint64_t count, const unsigned char* out)
{

}