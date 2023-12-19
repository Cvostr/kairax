#include "nvme.h"
#include "stdio.h"
#include "string.h"
#include "mem/kheap.h"
#include "kstdlib.h"
#include "mem/pmm.h"
#include "dev/device_drivers.h"
#include "dev/device.h"
#include "mem/iomem.h"

struct nvme_queue* nvme_create_queue(size_t slots)
{
	struct nvme_queue* queue = kmalloc(sizeof(struct nvme_queue));

	uint32_t submit_reqd_size = align(sizeof(struct nvme_command) * slots, PAGE_SIZE);
	queue->submit = P2V(pmm_alloc_pages(submit_reqd_size / PAGE_SIZE));
	memset(queue->submit, 0, sizeof(struct nvme_command) * slots);

	uint32_t compl_reqd_size = align(sizeof(struct nvme_compl_entry) * slots, PAGE_SIZE);
	queue->completion = P2V(pmm_alloc_pages(compl_reqd_size / PAGE_SIZE));
	memset(queue->completion, 0, sizeof(struct nvme_compl_entry) * slots);

	return queue;
}

int nvme_ctlr_device_probe(struct device *dev) {
	struct pci_device_info* device_desc = dev->pci_info;

	printf("NVME controller found on bus: %i, device: %i func: %i IRQ: %i \n", 
		device_desc->bus,
		device_desc->device,
		device_desc->function,
		device_desc->interrupt_line);

	struct nvme_device* device = (struct nvme_device*)kmalloc(sizeof(struct nvme_device));
	memset(device, 0, sizeof(struct nvme_device));

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
	//printf("STRIDE %i, QUEUES %i\n", device->stride, device->queues_num);

	// Создание admin queue
	device->admin_queue = nvme_create_queue(device->queues_num);
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
			printf("ERROR: Can't initialize NVME controller on bus: %i, device: %i func: %i IRQ: %i\n",
				device_desc->bus,
				device_desc->device,
				device_desc->function,
				device_desc->interrupt_line);
			break;
		}
	}

	return 0;
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
	.pci_device_ids = &nvme_ids,
	.ops = &nvme_ops
};

void init_nvme()
{
	register_pci_device_driver(&nvme_driver);
}