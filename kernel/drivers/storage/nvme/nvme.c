#include "nvme.h"
#include "stdio.h"
#include "string.h"
#include "mem/kheap.h"
#include "kstdlib.h"
#include "mem/pmm.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "mem/iomem.h"

int nvme_next_ctrlr_index = 0;

struct nvme_queue* nvme_create_queue(struct nvme_controller* controller, uint32_t id, size_t slots)
{
	struct nvme_queue* queue = kmalloc(sizeof(struct nvme_queue));
	memset(queue, 0, sizeof(struct nvme_queue));
	queue->slots = slots;

	uint32_t submit_reqd_size = align(sizeof(struct nvme_command) * slots, PAGE_SIZE);
	queue->submit = map_io_region(pmm_alloc_pages(submit_reqd_size / PAGE_SIZE), submit_reqd_size);
	memset(queue->submit, 0, sizeof(struct nvme_command) * slots);

	uint32_t compl_reqd_size = align(sizeof(struct nvme_completion) * slots, PAGE_SIZE);
	queue->completion = map_io_region(pmm_alloc_pages(compl_reqd_size / PAGE_SIZE), compl_reqd_size);
	memset(queue->completion, 0, sizeof(struct nvme_completion) * slots);

	queue->submission_doorbell = ((uintptr_t) controller->bar0) + 0x1000 + (2 * id) * (4 << controller->stride);
	queue->completion_doorbell = ((uintptr_t) controller->bar0) + 0x1000 + (2 * id + 1) * (4 << controller->stride);

	return queue;
}

int nvme_ctlr_device_probe(struct device *dev) 
{
	struct pci_device_info* device_desc = dev->pci_info;

	printf("NVME controller found on bus: %i, device: %i func: %i \n", 
		device_desc->bus,
		device_desc->device,
		device_desc->function);

	struct nvme_controller* device = (struct nvme_controller*)kmalloc(sizeof(struct nvme_controller));
	memset(device, 0, sizeof(struct nvme_controller));
	device->index = nvme_next_ctrlr_index++;
	device->pci_device = device_desc;
	device->bar0 = (struct nvme_bar0*) map_io_region(device_desc->BAR[0].address, device_desc->BAR[0].size);

	pci_set_command_reg(device_desc, pci_get_command_reg(device_desc) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
			
	// Выключить контроллер если включен
	uint32_t conf = device->bar0->conf;
	uint32_t reinit_conf = (0 << 4) | (0 << 11) | (0 << 14) | (6 << 16) | (4 << 20) | (1 << 0);
	if (conf & (1 << 0)) {
		conf &= ~(1 << 0);
		device->bar0->conf = conf;
	}

	while ((device->bar0->status) & (1 << 0));

	device->stride 		= (((device->bar0->cap) >> 32) & 0xf);
	device->queues_num 	= device->bar0->cap & 0xFFFF;
	//printf("NVME: stride %i, queues %i\n", device->stride, device->queues_num);

	// Создание admin queue
	device->admin_queue = nvme_create_queue(device, 0, device->queues_num);
	// Установка адресов admin queue
	device->bar0->a_submit_queue = (uint64_t)V2P(device->admin_queue->submit);
	device->bar0->a_compl_queue = (uint64_t)V2P(device->admin_queue->completion);

	uint32_t aqa = device->queues_num - 1;
	aqa |= aqa << 16;
	//aqa |= aqa << 16;
	device->bar0->aqa = aqa;

	// Включение контроллера
	device->bar0->conf = reinit_conf;
	while (1) {

		uint32_t status = device->bar0->status;
		if (status & (1 << 0)) {
			break; // ready
		}

		if (status & (1 << 1)) {
			printf("ERROR: Can't initialize NVME controller on bus: %i, device: %i func: %i\n",
				device_desc->bus,
				device_desc->device,
				device_desc->function);
			break;
		}
	}

	int rc = nvme_controller_identify(device);
	if (rc != 0) {
		return rc;
	}

	printk("NVME: Ctrlr name: '%s'\n", device->controller_id.model);
	//printk("Num namespaces: %i\n", device->controller_id.namespaces_num);

	if (device->controller_id.namespaces_num == 0) {
		printk("NVME: Controller has no namespaces\n");
		return 0;
	}

	char* ns_identity_buffer = pmm_alloc_page();
	memset(P2V(ns_identity_buffer), 0, PAGE_SIZE);

	struct nvme_command identify_cmd;
    memset(&identify_cmd, 0, sizeof(struct nvme_command));
	identify_cmd.opcode = NVME_CMD_IDENTIFY;
	identify_cmd.idcmd.id_type = IDCMD_NAMESPACE;
    identify_cmd.prp[0] = ns_identity_buffer;

	struct nvme_completion completion;

	for (int i = 0; i < device->controller_id.namespaces_num; i ++) 
	{
		identify_cmd.namespace_id = i + 1;
		memset(&completion, 0, sizeof(struct nvme_completion));

		nvme_queue_submit_wait(device->admin_queue, &identify_cmd, &completion);

		if (completion.status > 0) {
			printk("NVME: Namespace %i Identity failed! status = %i\n", i, completion.status);
			continue;
		}
		
		struct nvme_namespace_id* nsid = (struct nvme_namespace_id*) P2V(ns_identity_buffer);
		if (nsid->ns_size < 1) {
			continue;
		}

		struct nvme_namespace* ns = nvme_namespace(device, i + 1, nsid);

		struct drive_device_info* drive_info = kmalloc(sizeof(struct drive_device_info));
		memset(drive_info, 0, sizeof(struct drive_device_info));
		drive_info->nbytes = ns->disk_size;
		drive_info->sectors = drive_info->nbytes / ns->block_size; // ? 

		strcpy(drive_info->blockdev_name, "nvme");
		strcat(drive_info->blockdev_name, itoa(device->index, 10));
		strcat(drive_info->blockdev_name, "n");
		strcat(drive_info->blockdev_name, itoa(ns->namespace_id, 10));

		/*struct device* drive_dev = new_device();
		drive_dev->dev_type = DEVICE_TYPE_DRIVE;
		drive_dev->dev_parent = dev;
		drive_dev->dev_data = ns;
		drive_dev->drive_info = drive_info;
		
		drive_info->uses_lba48 = 1;*/
		//drive_info->write = ahci_port_write_lba;
		//drive_info->read = ahci_port_read_lba;
	}

	pmm_free_page(ns_identity_buffer);

	return 0;
}

int nvme_controller_identify(struct nvme_controller* controller)
{
	char* identity_buffer = pmm_alloc_page();
	memset(P2V(identity_buffer), 0, PAGE_SIZE);

	struct nvme_command identify_cmd;
    memset(&identify_cmd, 0, sizeof(struct nvme_command));

    identify_cmd.opcode = NVME_CMD_IDENTIFY;
    identify_cmd.prp[0] = identity_buffer;

    identify_cmd.idcmd.id_type = IDCMD_CONTROLLER;

    struct nvme_completion completion;
	memset(&completion, 0, sizeof(struct nvme_completion));
    nvme_queue_submit_wait(controller->admin_queue, &identify_cmd, &completion);

	if (completion.status > 0) {
		printk("NVME: Controller Identity failed! status = %i\n", completion.status);
		return -1;
	}

	memcpy(&controller->controller_id, P2V(identity_buffer), sizeof(struct nvme_controller_id));
	pmm_free_page(identity_buffer);

	return 0;
}

void nvme_queue_submit_wait(struct nvme_queue* queue, struct nvme_command* cmd, struct nvme_completion* compl)
{
	cmd->cmd_id = queue->next_cmd_id++;
    if (queue->next_cmd_id == UINT16_MAX) {
        queue->next_cmd_id = 0;
    }

	memcpy(&queue->submit[queue->submit_tail], cmd, sizeof(struct nvme_command));

	queue->submit_tail++;
    if (queue->submit_tail >= queue->slots) {
        queue->submit_tail = 0;
    }

	*queue->submission_doorbell = queue->submit_tail;

	int complflag = 0;
	while (complflag = !(queue->completion[queue->complete_head].phase_tag)) {
		asm volatile ("nop");
	}

	// Копируем результат
	memcpy(compl, &queue->completion[queue->complete_head], sizeof(struct nvme_completion));

	if (++queue->complete_head >= queue->slots) {
        queue->complete_head = 0;
    }

    *queue->completion_doorbell = queue->complete_head;
}

struct pci_device_id nvme_ids[] = {
	{PCI_DEVICE_CLASS(0x1, 0x8, 0)},
	{0,}
};

struct device_driver_ops nvme_ops = {

    .probe = nvme_ctlr_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_driver nvme_driver = {
	.dev_name = "Mass storage NVME Controller",
	.pci_device_ids = nvme_ids,
	.ops = &nvme_ops
};

void init_nvme()
{
	register_pci_device_driver(&nvme_driver);
}