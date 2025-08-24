#include "nvme.h"
#include "kairax/stdio.h"
#include "kairax/string.h"
#include "module.h"
#include "functions.h"
//#include "kstdlib.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "mem/iomem.h"
#include "dev/device_man.h"
#include "dev/interrupts.h"

//#define NVME_LOG_CONTROLLER_ID
//#define NVME_LOG_QUEUES_STRIDE
//#define NVME_LOG_QUEUES

int nvme_next_ctrlr_index = 0;

uint32_t* nvme_calc_submission_doorbell_addr(struct nvme_controller* controller, int id)
{
	return (uint32_t*) (((uintptr_t) controller->bar0) + 0x1000 + (2 * id) * (4 << controller->stride));
}

uint32_t* nvme_calc_completion_doorbell_addr(struct nvme_controller* controller, int id)
{
	return (uint32_t*) (((uintptr_t) controller->bar0) + 0x1000 + (2 * id + 1) * (4 << controller->stride));
}

void nvme_irq_handler(void* frame, void* data) 
{
	printk("NVME: Interrupt\n");
}

struct nvme_queue* nvme_create_admin_queue(struct nvme_controller* controller, size_t slots)
{
	struct nvme_queue* queue = kmalloc(sizeof(struct nvme_queue));
	memset(queue, 0, sizeof(struct nvme_queue));
	queue->slots = slots;
	queue->awaited_phase_flag = 1;

	// Выделить страницы под команды
	uint32_t reqd_size = align(sizeof(struct nvme_command) * slots, PAGE_SIZE);
	uint32_t reqd_pages = reqd_size / PAGE_SIZE;
	queue->submit_phys = pmm_alloc_pages(reqd_pages);
	queue->submit = map_io_region(queue->submit_phys, reqd_size);
	memset(queue->submit, 0, sizeof(struct nvme_command) * slots);

	// Выделить страницы под completion
	reqd_size = align(sizeof(struct nvme_completion) * slots, PAGE_SIZE);
	reqd_pages = reqd_size / PAGE_SIZE;
	queue->completion_phys = pmm_alloc_pages(reqd_pages);
	queue->completion = map_io_region(queue->completion_phys, reqd_size);
	memset(queue->completion, 0, sizeof(struct nvme_completion) * slots);

	queue->submission_doorbell = nvme_calc_submission_doorbell_addr(controller, 0);
	queue->completion_doorbell = nvme_calc_completion_doorbell_addr(controller, 0);

	return queue;
}

int nvme_allocate_io_queues(struct nvme_controller* controller, int count, int* allocated)
{
	// подготавливаем структуры команды и ожидания
	struct nvme_command set_features_cmd;
    memset(&set_features_cmd, 0, sizeof(struct nvme_command));
	set_features_cmd.opcode = NVME_CMD_SET_FEATURES;

	struct nvme_completion completion;
	memset(&completion, 0, sizeof(struct nvme_completion));

	set_features_cmd.setfeatures.feature_id = 0x7;
	set_features_cmd.setfeatures.spc11 = count << 16 | count;

	nvme_queue_submit_wait(controller->admin_queue, &set_features_cmd, &completion);

	if (completion.status > 0) {
		printk("NVME: Controller Allocating queues failed! status = %i\n", completion.status);
		return -1;
	}

	int compl_queues_allocated = (completion.res >> 16) & 0xffff; // High word
    int submission_queues_allocated = completion.res & 0xffff; 

	*allocated = MIN(compl_queues_allocated, submission_queues_allocated) + 1; 

	return 0;
}

struct nvme_queue* nvme_create_io_queue(struct nvme_controller* controller, size_t slots)
{
	struct nvme_queue* queue = kmalloc(sizeof(struct nvme_queue));
	memset(queue, 0, sizeof(struct nvme_queue));
	queue->slots = slots;
	queue->awaited_phase_flag = 1;

	// Ищем свободный номер очереди
	int queue_id = -1;		// 1 - ...
	for (int i = 0; i < controller->allocated_io_queues; i ++) 
	{
		if (controller->io_queues[i] == NULL) {
			queue_id = i + 1;
			break;
		}
	}

	if (queue_id == -1) {
		printk("Queue limit exceed\n");
		return NULL;
	}

	//printk("NVME: Queue id %i\n", queue_id);
	queue->id = queue_id;

	// подготавливаем структуры команды и ожидания
	struct nvme_command create_queue_cmd;
    memset(&create_queue_cmd, 0, sizeof(struct nvme_command));

	struct nvme_completion create_queue_completion;
	memset(&create_queue_completion, 0, sizeof(struct nvme_completion));

	// Выделить страницы под команды
	uint32_t reqd_size = align(sizeof(struct nvme_command) * slots, PAGE_SIZE);
	uint32_t reqd_pages = reqd_size / PAGE_SIZE;
	char* submit_queue_phys = pmm_alloc_pages(reqd_pages);
	queue->submit = map_io_region(submit_queue_phys, reqd_size);
	memset(queue->submit, 0, sizeof(struct nvme_command) * slots);

	// Выделить страницы под completion
	reqd_size = align(sizeof(struct nvme_completion) * slots, PAGE_SIZE);
	reqd_pages = reqd_size / PAGE_SIZE;
	char* complete_queue_phys = pmm_alloc_pages(reqd_pages);
	queue->completion = map_io_region(complete_queue_phys, reqd_size);
	memset(queue->completion, 0, sizeof(struct nvme_completion) * slots);

	// Сначала создаем completion queue
	create_queue_cmd.opcode = NVME_CMD_CREATE_IO_COMPLETION_QUEUE;
	create_queue_cmd.iocqcreate.contiguous = 1;
	create_queue_cmd.iocqcreate.queue_id = queue_id;
	create_queue_cmd.iocqcreate.queue_size = slots - 1;
	create_queue_cmd.prp[0] = complete_queue_phys;

	nvme_queue_submit_wait(controller->admin_queue, &create_queue_cmd, &create_queue_completion);

	if (create_queue_completion.status > 0) {
		printk("NVME: Failed creating IO complete queue(%i)! status = %i\n", queue_id, create_queue_completion.status);
		return NULL;
	}

	// Создаем submission queue
	create_queue_cmd.opcode = NVME_CMD_CREATE_IO_SUBMISSION_QUEUE;
	create_queue_cmd.iosqcreate.contiguous = 1;
	create_queue_cmd.iosqcreate.queue_id = queue_id;
	create_queue_cmd.iosqcreate.queue_size = slots - 1;
	create_queue_cmd.iosqcreate.completion_queue_id = queue_id;
	create_queue_cmd.prp[0] = submit_queue_phys;

	nvme_queue_submit_wait(controller->admin_queue, &create_queue_cmd, &create_queue_completion);

	if (create_queue_completion.status > 0) {
		printk("NVME: Failed creating IO submit queue(%i)! status = %i\n", queue_id, create_queue_completion.status);
		return NULL;
	}

	queue->submission_doorbell = nvme_calc_submission_doorbell_addr(controller, queue_id);
	queue->completion_doorbell = nvme_calc_completion_doorbell_addr(controller, queue_id);

	return queue;
}

int nvme_read_lba(struct device* dev, uint64_t start, uint64_t count, unsigned char *buf)
{
    return nvme_namespace_read(dev->dev_data, start, (uint32_t)count, (uint16_t*) buf);
}

int nvme_write_lba(struct device* dev, uint64_t start, uint64_t count, const unsigned char *buf)
{
    return nvme_namespace_write(dev->dev_data, start, (uint32_t)count, (uint16_t*) buf);
}

int nvme_ctlr_device_probe(struct device *dev) 
{
	struct pci_device_info* device_desc = dev->pci_info;

#ifdef NVME_LOG_CONTROLLER_ID
	printk("NVME controller found on bus: %i, device: %i func: %i \n", 
		device_desc->bus,
		device_desc->device,
		device_desc->function);
#endif

	struct nvme_controller* device = (struct nvme_controller*)kmalloc(sizeof(struct nvme_controller));
	memset(device, 0, sizeof(struct nvme_controller));
	device->index = nvme_next_ctrlr_index++;
	device->pci_device = device_desc;
	device->bar0 = (struct nvme_bar0*) map_io_region(device_desc->BAR[0].address, device_desc->BAR[0].size);
	//printk("BAR0 %s SIZE %i\n", ulltoa(device_desc->BAR[0].address, 16), device_desc->BAR[0].size);

	pci_set_command_reg(device_desc, pci_get_command_reg(device_desc) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);

	// Выключить контроллер если включен
	uint32_t conf = device->bar0->conf;
	uint32_t reinit_conf = (0 << 4) | (0 << 11) | (0 << 14) | (6 << 16) | (4 << 20) | (1 << 0);
	if (conf & (1 << 0)) {
		conf &= ~(1 << 0);
		device->bar0->conf = conf;
	}

	while ((device->bar0->status) & (1 << 0));

	int min_page_size = 0x1000 << ((device->bar0->cap >> 48) & 0xF);
	int max_page_size = 0x1000 << ((device->bar0->cap >> 52) & 0xF);

	if (min_page_size > PAGE_SIZE || max_page_size < PAGE_SIZE) {
		printk("NVME: Controller has incorrect page size range (%i - %i)\n", min_page_size, max_page_size);
		return 0;
	}

	device->stride 		= (((device->bar0->cap) >> 32) & 0xf);
	device->queue_entries_num 	= (device->bar0->cap & 0xFFFF) + 1; // Значение, начинающееся с нуля
#ifdef NVME_LOG_QUEUES_STRIDE
	printk("NVME: stride %i, queue_entries %i\n", device->stride, device->queue_entries_num);
#endif

	// Создание admin queue
	device->admin_queue = nvme_create_admin_queue(device, device->queue_entries_num);
	// Установка адресов admin queue
	device->bar0->a_submit_queue = (uint64_t)(device->admin_queue->submit_phys);
	device->bar0->a_compl_queue = (uint64_t)(device->admin_queue->completion_phys);

	uint32_t aqa = device->queue_entries_num - 1;
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

	for(int p = strlen(device->controller_id.model) - 1; p > 0; p --){
		if(device->controller_id.model[p] == ' ')
			device->controller_id.model[p] = '\0';
		else 
			break;
	}

	printk("NVME: Ctrlr name: '%s', %i namespaces\n", device->controller_id.model, device->controller_id.namespaces_num);

	int check1 = device->controller_id.controller_type == NVME_CONTROLLER_TYPE_IO;
	int check2 = device->controller_id.controller_type == NVME_CONTROLLER_TYPE_NOT_REPORTED;
	if (!(check1 || check2)) {
		printk("NVME: Controller has incorrect type (%i) !\n", device->controller_id.controller_type);
		return 0;
	}

	if (device->controller_id.namespaces_num == 0) {
		printk("NVME: Controller has no namespaces\n");
		return 0;
	}

	// Выделить на контроллере требуемое количество очередей (0 - ...)
	nvme_allocate_io_queues(device, NVME_CONTROLLER_MAX_IO_QUEUES, &device->allocated_io_queues);
#ifdef NVME_LOG_QUEUES
	printk("NVME: Allocated queues: %i\n", device->allocated_io_queues);
#endif

	device->io_queues = kmalloc(device->allocated_io_queues * sizeof(struct nvme_queue*));

	int i = 0;
	for (i = 0; i < device->allocated_io_queues; i ++) {
		struct nvme_queue* queue = nvme_create_io_queue(device, device->queue_entries_num);
		device->io_queues[queue->id - 1] = queue;
	}

    if (pci_device_is_msix_capable(dev->pci_info)) 
	{
		uint8_t irq = alloc_irq(0, "nvme");
		printk("NVME: Using IRQ %i\n", irq);
    	pci_device_set_msix_vector(dev, irq);
		register_irq_handler(irq, nvme_irq_handler, device);
    }

	char* ns_identity_buffer = pmm_alloc_page();
	memset(vmm_get_virtual_address(ns_identity_buffer), 0, PAGE_SIZE);

	struct nvme_command identify_cmd;
    memset(&identify_cmd, 0, sizeof(struct nvme_command));
	identify_cmd.opcode = NVME_CMD_IDENTIFY;
	identify_cmd.idcmd.id_type = IDCMD_NAMESPACE;
    identify_cmd.prp[0] = ns_identity_buffer;

	struct nvme_completion completion;

	for (i = 0; i < device->controller_id.namespaces_num; i ++) 
	{
		identify_cmd.namespace_id = i + 1;
		memset(&completion, 0, sizeof(struct nvme_completion));

		nvme_queue_submit_wait(device->admin_queue, &identify_cmd, &completion);

		if (completion.status > 0) {
			printk("NVME: Namespace %i Identity failed! status = %i\n", i, completion.status);
			continue;
		}
		
		struct nvme_namespace_id* nsid = (struct nvme_namespace_id*) vmm_get_virtual_address(ns_identity_buffer);
		if (nsid->ns_size < 1) {
			continue;
		}

		struct nvme_namespace* ns = nvme_namespace(device, i + 1, nsid);

		struct drive_device_info* drive_info = kmalloc(sizeof(struct drive_device_info));
		memset(drive_info, 0, sizeof(struct drive_device_info));
		drive_info->block_size = ns->block_size;
		drive_info->nbytes = ns->disk_size * drive_info->block_size;
		drive_info->sectors = ns->disk_size;
		drive_info->uses_lba48 = 1; // ???

		drive_info->read = nvme_read_lba;
		drive_info->write = nvme_write_lba;

		strcpy(drive_info->blockdev_name, "nvme");
		strcat(drive_info->blockdev_name, itoa(device->index, 10));
		strcat(drive_info->blockdev_name, "n");
		strcat(drive_info->blockdev_name, itoa(ns->namespace_id, 10));

		struct device* drive_dev = new_device();
		drive_dev->dev_type = DEVICE_TYPE_DRIVE;
		device_set_parent(drive_dev, dev);
		drive_dev->dev_data = ns;
		drive_dev->drive_info = drive_info;

		register_device(drive_dev);
	}

	pmm_free_page(ns_identity_buffer);

	return 0;
}

int nvme_controller_identify(struct nvme_controller* controller)
{
	char* identity_buffer = pmm_alloc_page();
	memset(vmm_get_virtual_address(identity_buffer), 0, PAGE_SIZE);

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

	memcpy(&controller->controller_id, vmm_get_virtual_address(identity_buffer), sizeof(struct nvme_controller_id));
	pmm_free_page(identity_buffer);

	controller->controller_id.model[39] = 0;

	return 0;
}

struct nvme_queue* nvme_ctrlr_acquire_queue(struct nvme_controller* controller)
{
	// TODO: блокировка, выбор многих
	return controller->io_queues[0];
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

	// Запуск команды на исполнение
	*queue->submission_doorbell = queue->submit_tail;

	while (queue->awaited_phase_flag == !(queue->completion[queue->complete_head].phase_tag)) {
		asm volatile ("nop");
	}

	// Копируем результат
	memcpy(compl, &queue->completion[queue->complete_head], sizeof(struct nvme_completion));

	if (++queue->complete_head >= queue->slots) {
        queue->complete_head = 0;
		// Идем на новый круг, теперь ожидаем инвертированный phase tag
		queue->awaited_phase_flag = !queue->awaited_phase_flag;
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

int init_nvme(void)
{
    register_pci_device_driver(&nvme_driver);
	
	return 0;
}

void nvme_exit(void)
{

}

DEFINE_MODULE("nvme", init_nvme, nvme_exit)