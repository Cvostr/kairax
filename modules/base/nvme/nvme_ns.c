#include "nvme.h"
#include "functions.h"
#include "kairax/string.h"
#include "dev/device.h"
#include "mem/iomem.h"
#include "mem/vmm.h"
#include "kairax/stdio.h"
#include "kairax/errors.h"
#include "kairax/kstdlib.h"

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
    struct nvme_command read_cmd;
    struct nvme_completion completion;

    // Захватываем свободную очередь
    struct nvme_queue *queue = nvme_ctrlr_acquire_queue(ns->controller);

    size_t blocksize = ns->block_size;
    // сколько байт можно максимум считать одной командой
    size_t cmd_max_bytes = 4096;
    // сколько блоков можно считать одной командой
    size_t lba_per_cmd = cmd_max_bytes / blocksize;

    size_t remain_lba = count;
    size_t readed_lba = 0;

    while (remain_lba > 0)
    {
        // сколько читаем за этот раз
        size_t lba_to_read = MIN(remain_lba, lba_per_cmd);

        // Формируем команду
        memset(&read_cmd, 0, sizeof(struct nvme_command));
        read_cmd.opcode = NVME_CMD_IO_READ;
        read_cmd.namespace_id = ns->namespace_id;
        read_cmd.rdcmd.start_lba = lba;
        read_cmd.rdcmd.block_count = lba_to_read - 1;
        read_cmd.prp[0] = queue->buffer_phys;

        // Отправить на выполнение и ожидать завершения
        memset(&completion, 0, sizeof(struct nvme_completion));
        nvme_queue_submit_wait(queue, &read_cmd, &completion);

        if (completion.status > 0) {
            printk("NVME: Error reading %i. lba %i, count %i\n", completion.status, lba, lba_to_read);
            nvme_release_queue(queue);
            return -EIO;
        }

        // копирование из промежуточной страницы 
        memcpy(out + readed_lba * blocksize, queue->buffer, lba_to_read * blocksize);

        // обновить счетчики
        lba += lba_to_read;
        remain_lba -= lba_to_read;
        readed_lba += lba_to_read;
    }

    // освобождаем очередь
    nvme_release_queue(queue);

    return 0;
}

int nvme_namespace_write(struct nvme_namespace* ns, uint64_t lba, uint64_t count, const unsigned char* in)
{
    struct nvme_command write_cmd;
    struct nvme_completion completion;

    // Захватываем свободную очередь
    struct nvme_queue *queue = nvme_ctrlr_acquire_queue(ns->controller);

    // Формируем команду
    memset(&write_cmd, 0, sizeof(struct nvme_command));
    write_cmd.opcode = NVME_CMD_IO_WRITE;
    write_cmd.namespace_id = ns->namespace_id;
    write_cmd.wrcmd.start_lba = lba;
    write_cmd.wrcmd.block_count = count - 1;
    write_cmd.prp[0] = queue->buffer_phys;

    // копирование в промежуточную страницу
    memcpy(queue->buffer, in, count * 512);

    // Отправить на выполнение и ожидать завершения
    memset(&completion, 0, sizeof(struct nvme_completion));
    nvme_queue_submit_wait(queue, &write_cmd, &completion);

    if (completion.status > 0) {
        printk("NVME: Error writing %i. lba %i, count %i\n", completion.status, lba, count);
        nvme_release_queue(queue);
        return -EIO;
    }

    // освобождаем очередь
    nvme_release_queue(queue);

    return 0;
}