#include "nvme.h"
#include "mem/kheap.h"
#include "kairax/string.h"
#include "mem/pmm.h"
#include "dev/device.h"
#include "mem/iomem.h"
#include "mem/vmm.h"
#include "stdio.h"

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

    ns->buffer_phys = (uintptr_t) pmm_alloc_page();
	ns->buffer = (char*) map_io_region(ns->buffer_phys, PAGE_SIZE);

    printk("NVME: Namespace %i: lba_sz %i blk_sz %i size %i\n", id, ns->lba_size, ns->block_size, ns->disk_size);

    return ns;
}

int nvme_read_lba(struct device* dev, uint64_t start, uint64_t count, unsigned char *buf)
{
    return nvme_namespace_read(dev->dev_data, start, (uint32_t)count, (uint16_t*) buf);
}

int nvme_namespace_read(struct nvme_namespace* ns, uint64_t lba, uint64_t count, unsigned char* out)
{
    struct queue* queue = nvme_ctrlr_acquire_queue(ns->controller);

    acquire_spinlock(&ns->lock);

    struct nvme_command read_cmd;
    memset(&read_cmd, 0, sizeof(struct nvme_command));
	read_cmd.opcode = NVME_CMD_IO_READ;
    read_cmd.namespace_id = ns->namespace_id;
	read_cmd.rdcmd.start_lba = lba;
    read_cmd.rdcmd.block_count = count - 1;
    read_cmd.prp[0] = ns->buffer_phys;

	struct nvme_completion completion;
    memset(&completion, 0, sizeof(struct nvme_completion));

    nvme_queue_submit_wait(queue, &read_cmd, &completion);

    if (completion.status > 0) {
        printk("Error reading\n");
        return -1;
    }

    memcpy(out, ns->buffer, count * 512);

    release_spinlock(&ns->lock);

    return 0;
}

int nvme_write_lba(struct device* dev, uint64_t start, uint64_t count, const unsigned char *buf)
{
    return nvme_namespace_write(dev->dev_data, start, (uint32_t)count, (uint16_t*) buf);
}

int nvme_namespace_write(struct nvme_namespace* ns, uint64_t lba, uint64_t count, const unsigned char* in)
{
    struct queue* queue = nvme_ctrlr_acquire_queue(ns->controller);

    acquire_spinlock(&ns->lock);

    struct nvme_command read_cmd;
    memset(&read_cmd, 0, sizeof(struct nvme_command));
    read_cmd.opcode = NVME_CMD_IO_WRITE;
    read_cmd.namespace_id = ns->namespace_id;
    read_cmd.wrcmd.start_lba = lba;
    read_cmd.wrcmd.block_count = count - 1;
    read_cmd.prp[0] = ns->buffer_phys;

    struct nvme_completion completion;
    memset(&completion, 0, sizeof(struct nvme_completion));

    memcpy(ns->buffer, in, count * 512);

    nvme_queue_submit_wait(queue, &read_cmd, &completion);

    if (completion.status > 0) {
        printk("Error writing\n");
        return -1;
    }

    release_spinlock(&ns->lock);

    return 0;
}