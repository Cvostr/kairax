#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/interrupts.h"
#include "hda.h"
#include "mem/iomem.h"

void hda_device_init_streams(struct hda_dev* dev_data) 
{
    int stream_size = BDL_SIZE * HDA_STREAM_BUFFER_SIZE;
    int stream_pages = stream_size / PAGE_SIZE;

    void* istream = pmm_alloc_pages(stream_pages);
    void* ostream = pmm_alloc_pages(stream_pages);

    dev_data->istream = (void*) map_io_region(istream, stream_size);
    dev_data->ostream = (void*) map_io_region(ostream, stream_size);

    hda_outd(dev_data, ISTREAM_CTLL, 0U);
    hda_outd(dev_data, ISTREAM_CTLL, 1);
    while ((hda_ind(dev_data, ISTREAM_CTLL) & 0x1) == 0)
        ;

    
}

void hda_setup_corb_rirb(struct hda_dev* dev_data)
{
    void* corb = pmm_alloc_pages(1);
    void* rirb = pmm_alloc_pages(1);

    dev_data->corb = (uint32_t*) map_io_region(corb, PAGE_SIZE);
    dev_data->rirb = (uint64_t*) map_io_region(rirb, PAGE_SIZE);

    uint32_t corbsizereg = hda_inb(dev_data, CORBSIZE);
    uint32_t rirbsizereg = hda_inb(dev_data, RIRBSIZE);

    if (corbsizereg & (1 << 6)) {
        dev_data->corb_entries = 256;
        corbsizereg |= 0x2;
    } else if (corbsizereg & (1 << 5)) {
        dev_data->corb_entries = 16;
        corbsizereg |= 0x1;
    } else if (corbsizereg & (1 << 4)) {
        dev_data->corb_entries = 2;
    } 

    printk("CORB entries : %i\n", dev_data->corb_entries);

    // CORB - установка адреса
    hda_outd(dev_data, CORBLBASE, (uint32_t)corb);
    hda_outd(dev_data, CORBUBASE, ((uintptr_t)corb >> 32));
    hda_outb(dev_data, CORBSIZE, corbsizereg);

    // сброс указателя RP CORB
    hda_outw(dev_data, CORBRP, (1 << 15));
    while ((hda_inw(dev_data, CORBRP) & (1 << 15)) == 0);

    hda_outw(dev_data, CORBRP, 0);
    while (hda_inw(dev_data, CORBRP) != 0);
    
    // WP
    hda_outw(dev_data, CORBWP, 0);

    if (rirbsizereg & (1 << 6)) {
        dev_data->rirb_entries = 256;
        rirbsizereg |= 0x2;
    } else if (rirbsizereg & (1 << 5)) {
        dev_data->rirb_entries = 16;
        rirbsizereg |= 0x1;
    } else if (rirbsizereg & (1 << 4)) {
        dev_data->rirb_entries = 2;
    } 

    printk("RIRB entries : %i\n", dev_data->rirb_entries);

    // RIRB - установка адреса
    hda_outd(dev_data, RIRBLBASE, (uint32_t)rirb);
    hda_outd(dev_data, RIRBUBASE, ((uintptr_t)rirb >> 32));
    hda_outb(dev_data, RIRBSIZE, rirbsizereg);
    
    // Сброс указателя RIRB WP
    hda_outd(dev_data, RIRBWP, (1 << 15));
    hda_outw(dev_data, RIRBWP, 0);
}

int hda_device_probe(struct device *dev) 
{
    printk("Intel HD Audio Device found!\n");

    pci_set_command_reg(dev->pci_info, pci_get_command_reg(dev->pci_info) | PCI_DEVCMD_BUSMASTER_ENABLE | PCI_DEVCMD_MSA_ENABLE);
    pci_device_set_enable_interrupts(dev->pci_info, 1);

    struct hda_dev* dev_data = kmalloc(sizeof(struct hda_dev));
    dev_data->io_addr = map_io_region(dev->pci_info->BAR[0].address, dev->pci_info->BAR[0].size);

    dev->dev_data = dev_data;

    uint8_t irq = dev->pci_info->interrupt_line;
	register_irq_handler(irq, hda_int_handler, dev_data);

    printk("Using IRQ : %i\n", irq);

    // Включение по CRST
    hda_outd(dev_data, GCTL, 1);
    while (hda_ind(dev_data, GCTL) != 0x1);

    // Получить информацию о количестве стримов
    uint16_t caps = hda_inw(dev_data, GCAP);
    dev_data->supports64 = caps & 0x01; 
    dev_data->iss_num = (caps & 0xF000) >> 12;
    dev_data->oss_num = (caps & 0x0F00) >> 8;
    dev_data->bss_num = (caps & 0x00F8) >> 3;

    // сразу вычислим регистр первого output stream
    dev_data->ostream_reg_base = ISTREAM_CTLL + (0x20 * dev_data->iss_num);

    printk("Device CAPS: iss %i oss %i bss %i\n", dev_data->iss_num, dev_data->oss_num, dev_data->bss_num);

    hda_outd(dev_data, CORBCTL, 0);
    hda_outd(dev_data, RIRBCTL, 0);

    delay();

    hda_outd(dev_data, DPLBASE, 0);
    hda_outd(dev_data, DPUBASE, 0);

    // Выключение прерываний
    hda_outd(dev_data, INTCTL, 0);

    // Инициализация CORB & RIRB
    hda_setup_corb_rirb(dev_data);

    hda_outd(dev_data, WAKEEN, hda_ind(dev_data, WAKEEN) & ~0x7fU);
    // Включение прерываний
    hda_outd(dev_data, GCTL, hda_ind(dev_data, GCTL) | (1 << 8));
    hda_outd(dev_data, INTCTL, (1U << 31) | (1U << 30));

    // RIRB response interrupt count
    hda_outd(dev_data, RINTCNT, 1);

    // Запуск CORB
    uint8_t corbctl = hda_inb(dev_data, CORBCTL);
    corbctl |= 0x3;
    hda_outd(dev_data, CORBCTL, corbctl);

    // Запуск RIRB
    uint8_t rirbctl = hda_inb(dev_data, RIRBCTL);
    rirbctl |= 0x3;
    hda_outd(dev_data, RIRBCTL, rirbctl);

    // Ждем пока на устройстве регистрируются кодеки    
    delay();

    uint32_t status = hda_ind(dev_data, STATESTS);
    printk("STATESTS %i\n", status);

    for (int codec_i = 0; codec_i < HDA_MAX_CODECS; codec_i ++) {
        if (status & (1 >> codec_i)) {
            // init codec
            
            uint32_t vendor = hda_codec_get_param(dev_data, codec_i, 0, HDA_CODEC_PARAM_VENDOR_ID);
            uint32_t rev = hda_codec_get_param(dev_data, codec_i, 0, HDA_CODEC_PARAM_REVISION_ID);
            uint32_t sub = hda_codec_get_param(dev_data, codec_i, 0, HDA_CODEC_PARAM_SUB_NODE_COUNT);
            printk("Codec %i : vendor %i, revision %i, sub nodes %i\n", codec_i, vendor, rev, sub);
        }
    }

    hda_device_init_streams(dev_data);

    printk("HDA device initialized\n");

    return 0;
}

uint32_t hda_codec_get_param(struct hda_dev* dev, int codec, uint32_t node, uint32_t param)
{
    uint64_t out = 0;
    uint32_t verb = hda_make_verb(codec, node, (HDA_VERB_CODEC_GET_PARAM << 8) | param);
    hda_codec_send_verb(dev, verb, &out);

    uint32_t value = out & 0xffffffff;

    if (out >> 32 != codec) {
        printk("hda_codec_get_param : wrong result\n");
    }

    return value;
}

int hda_codec_send_verb(struct hda_dev* dev, uint32_t verb, uint64_t* out)
{
    uint16_t corbwp = hda_inw(dev, CORBWP);
    uint16_t rirbwp = hda_inw(dev, RIRBWP);

    corbwp = (corbwp + 1) % dev->corb_entries;

    printk("OLD %i %i %i %i\n", corbwp, hda_inw(dev, CORBWP), hda_inw(dev, RIRBWP), hda_inw(dev, CORBRP));

    dev->corb[corbwp] = verb;
    hda_outw(dev, CORBWP, corbwp);

    while (hda_inw(dev, RIRBWP) == rirbwp);

    *out = dev->rirb[(rirbwp + 1) % dev->rirb_entries];

    printk("NEW %i %i %i\n", hda_inw(dev, CORBWP), hda_inw(dev, RIRBWP), hda_inw(dev, CORBRP));

    return 0;
}

void hda_int_handler(void* regs, struct hda_dev* data) 
{
	printk("HDA INT ");
    hda_outb(data, RIRBSTS, hda_inb(data, RIRBSTS) | (4 | 1));
}

struct pci_device_id hda_pci_id[] = {
	{PCI_DEVICE_CLASS(0x04, 0x03, 0x00)},
	{0,}
};

struct device_driver_ops hda_ops = {

    .probe = hda_device_probe
    //void (*remove) (struct device *dev);
    //void (*shutdown) (struct device *dev);
};

struct pci_device_driver hda_driver = {
	.dev_name = "Intel HD Audio Controller",
	.pci_device_ids = hda_pci_id,
	.ops = &hda_ops
};

int init(void)
{
    register_pci_device_driver(&hda_driver);
	
	return 0;
}

void exit(void)
{

}

DEFINE_MODULE("hda", init, exit)