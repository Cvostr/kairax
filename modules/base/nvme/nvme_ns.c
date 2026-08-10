#include "nvme.h"
#include "functions.h"
#include "kairax/string.h"
#include "dev/device.h"
#include "mem/iomem.h"
#include "mem/vmm.h"
#include "kairax/stdio.h"
#include "kairax/errors.h"

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
    // Захватываем свободную очередь
    struct nvme_queue *queue = nvme_ctrlr_acquire_queue(ns->controller);

    struct nvme_command read_cmd;
    memset(&read_cmd, 0, sizeof(struct nvme_command));
	read_cmd.opcode = NVME_CMD_IO_READ;
    read_cmd.namespace_id = ns->namespace_id;
	read_cmd.rdcmd.start_lba = lba;
    read_cmd.rdcmd.block_count = count - 1;
    read_cmd.prp[0] = queue->buffer_phys;

	struct nvme_completion completion;
    memset(&completion, 0, sizeof(struct nvme_completion));

    nvme_queue_submit_wait(queue, &read_cmd, &completion);

    if (completion.status > 0) {
        printk("NVME: Error reading\n");
        return -EIO;
    }

    memcpy(out, queue->buffer, count * 512);

    // освобождаем очередь
    queue->acquired = FALSE;

    return 0;
}

int nvme_namespace_write(struct nvme_namespace* ns, uint64_t lba, uint64_t count, const unsigned char* in)
{
    // Захватываем свободную очередь
    struct nvme_queue *queue = nvme_ctrlr_acquire_queue(ns->controller);

    struct nvme_command read_cmd;
    memset(&read_cmd, 0, sizeof(struct nvme_command));
    read_cmd.opcode = NVME_CMD_IO_WRITE;
    read_cmd.namespace_id = ns->namespace_id;
    read_cmd.wrcmd.start_lba = lba;
    read_cmd.wrcmd.block_count = count - 1;
    read_cmd.prp[0] = queue->buffer_phys;

    struct nvme_completion completion;
    memset(&completion, 0, sizeof(struct nvme_completion));

    memcpy(queue->buffer, in, count * 512);

    nvme_queue_submit_wait(queue, &read_cmd, &completion);

    if (completion.status > 0) {
        printk("NVME: Error writing\n");
        return -EIO;
    }

    // освобождаем очередь
    queue->acquired = FALSE;

    return 0;
}