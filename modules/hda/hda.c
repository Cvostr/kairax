#include "module.h"
#include "functions.h"
#include "dev/device.h"
#include "dev/bus/pci/pci.h"
#include "dev/device_drivers.h"
#include "dev/interrupts.h"
#include "hda.h"
#include "mem/iomem.h"
#include "kairax/string.h"

struct hda_stream* hda_device_init_stream(struct hda_dev* dev, int index, int type, uint16_t format) 
{
    struct hda_stream* stream = kmalloc(sizeof(struct hda_stream));

    switch (type) {
        case 1: // Input
            stream->reg_base = ISTREAM_REG_BASE + (index - 1) * 0x20;
            break;
        case 2: // Output
            stream->reg_base = dev->ostream_reg_base + (index - 1) * 0x20;
            break;
    }

    // Сброс Stream Descriptor Control
    // Сначала пишем 1 для начала сброса
    hda_outw(dev, stream->reg_base + ISTREAM_CTLL, 1);
    while ((hda_inw(dev, stream->reg_base + ISTREAM_CTLL) & 0x1) == 0)
        ;

    // Потом уже 0, для выхода из сброса
    hda_outw(dev, stream->reg_base + ISTREAM_CTLL, 0);
    while ((hda_inw(dev, stream->reg_base + ISTREAM_CTLL) & HDA_STREAM_CTL_RST) != 0)
        ;

    // Максимальное количество BDL
    //stream->bdl_num = PAGE_SIZE / sizeof(struct HDA_BDL_ENTRY);
    stream->bdl_num = 2;

    // Выделение памяти под BDL - всегда 4 кб
    void* bdl_phys = pmm_alloc_pages(1);
    stream->bdl = (struct HDA_BDL_ENTRY*) map_io_region(bdl_phys, PAGE_SIZE);

    // Записать адрес BDL в регистры
    hda_outd(dev, stream->reg_base + ISTREAM_BDPL, (uint32_t) bdl_phys);
    hda_outd(dev, stream->reg_base + ISTREAM_BDPU, ((uintptr_t) bdl_phys) >> 32);

    // Вычисляем максимальный размер буфера
    stream->size = PAGE_SIZE * stream->bdl_num;

    hda_outd(dev, stream->reg_base + ISTREAM_CBL, stream->size);
	hda_outw(dev, stream->reg_base + ISTREAM_LVI, stream->bdl_num - 1); // позиция последнего BDL

    for (int bdl_i = 0; bdl_i < stream->bdl_num; bdl_i ++) {
        
        stream->bdl[bdl_i].length = PAGE_SIZE;
        stream->bdl[bdl_i].ioc = 1; // FIND OUT

        stream->bdl[bdl_i].paddr = pmm_alloc_pages(1);
        map_io_region(stream->bdl[bdl_i].paddr, PAGE_SIZE);
    }

    // Установка формата
	hda_outw(dev, stream->reg_base + ISTREAM_FMT, format);

    // Запись номера потока в третий байт
    uint8_t ctlu = hda_inb(dev, stream->reg_base + ISTREAM_CTLU);
    hda_outb(dev, stream->reg_base + ISTREAM_CTLU, ctlu | ((index & 0xf) << 0x4));

    // Очистка бит статуса
    hda_outb(dev, stream->reg_base + ISTREAM_STS, HDAC_SDSTS_DESE | HDAC_SDSTS_FIFOE | HDAC_SDSTS_BCIS);

    printk("Stream initialized\n");

    return stream;
}

void hda_stream_run(struct hda_dev* dev, struct hda_stream* stream)
{
    // Запуск DMA Engine для этого потока
    uint8_t value = hda_inb(dev, stream->reg_base + ISTREAM_CTLL);
	value |= HDA_STREAM_CTL_DEI;
	value |= HDA_STREAM_CTL_FEI;
	value |= HDA_STREAM_CTL_IOC; 
	value |= HDA_STREAM_CTL_RUN;
	hda_outb(dev, stream->reg_base + ISTREAM_CTLL, value);
}

uint32_t hda_stream_write(struct hda_stream* stream, char* mem, uint32_t size)
{
    uint32_t written = 0;
    uint32_t written_in_bdl = 0;
    int bdl_i = 0;
    struct HDA_BDL_ENTRY* current_bdl = stream->bdl;

    for (uint32_t i = 0; (i < size) && (bdl_i < stream->bdl_num); i ++) {

        char* dst = current_bdl->paddr;
        dst = vmm_get_virtual_address(dst);
        dst[written_in_bdl++] = mem[written++];
        //printk("AADDD");

        if (written_in_bdl >= current_bdl->length) {
            bdl_i++;
            current_bdl = &stream->bdl[bdl_i];
            written_in_bdl = 0;
        }
    }

    return written;
}

void hda_setup_corb_rirb(struct hda_dev* dev_data)
{
    void* corb = pmm_alloc_pages(1);
    void* rirb = pmm_alloc_pages(1);

    dev_data->corb = (uint32_t*) map_io_region(corb, PAGE_SIZE);
    dev_data->rirb = (uint64_t*) map_io_region(rirb, PAGE_SIZE);

    uint32_t corbsizereg = hda_inb(dev_data, CORBSIZE);
    uint32_t rirbsizereg = hda_inb(dev_data, RIRBSIZE);

    printk("CORB reg %i RIRB reg %i \n", corbsizereg, rirbsizereg);

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
    dev_data->ostream_reg_base = 0x80 + (0x20 * dev_data->iss_num);

    printk("Device CAPS: iss %i oss %i bss %i\n", dev_data->iss_num, dev_data->oss_num, dev_data->bss_num);

    // Выключение прерываний
    hda_outd(dev_data, INTCTL, 0);
    delay();

    // Остановить CORB, RIRB
    hda_outd(dev_data, CORBCTL, 0);
    hda_outd(dev_data, RIRBCTL, 0);

    // Ожидание остановки CORB, RIRB
    while (hda_ind(dev_data, CORBCTL) & 0x2 || hda_ind(dev_data, RIRBCTL) & 0x2)
        ;

    // Пока не используем DMA Position buffer
    // TODO: изучить и начать использовать, если необходимо
    hda_outd(dev_data, DPLBASE, 0);
    hda_outd(dev_data, DPUBASE, 0);

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
            struct hda_codec* codec = hda_determine_codec(dev_data, codec_i);
            if (codec) {
                dev_data->codecs[codec_i] = codec;
            }
        }
    }

    printk("HDA device initialized\n");

    return 0;
}

struct hda_codec* hda_determine_codec(struct hda_dev* dev, int codec)
{
    struct hda_codec* hcodec = kmalloc(sizeof(struct hda_codec));
    memset(hcodec, 0, sizeof(struct hda_codec));

    hcodec->codec = codec;
    //hcodec->pin_widgets = create_list();

    uint32_t vendor = hda_codec_get_param(dev, codec, 0, HDA_CODEC_PARAM_VENDOR_ID);

    if (vendor == 0xffffffff) {
        return NULL;
    }

    uint32_t rev = hda_codec_get_param(dev, codec, 0, HDA_CODEC_PARAM_REVISION_ID);
    uint32_t sub = hda_codec_get_param(dev, codec, 0, HDA_CODEC_PARAM_SUB_NODE_COUNT);

    hcodec->vendor_id = vendor >> 16;
    hcodec->device_id = vendor & 0xFFFF;

    hcodec->major = (rev >> 20) & 0xF;
    hcodec->minor = (rev >> 16) & 0xF;
    hcodec->revision = (rev >> 8) & 0xFF;
    hcodec->stepping = (rev) & 0xFF;

    hcodec->starting_node = (sub >> 16) & 0xFF;
    hcodec->nodes_num = (sub) & 0xFF;

    printk("Codec %i : vendor %i, device %i ver %i.%i.%i, start node %i, nodes %i\n",
        codec, 
        hcodec->vendor_id,
        hcodec->device_id,
        hcodec->major, 
        hcodec->minor,
        hcodec->revision,
        hcodec->starting_node,
        hcodec->nodes_num);

    // Ищем виджеты, прицепленные к этому кодеку
    for (int node_i = hcodec->starting_node; node_i < hcodec->starting_node + hcodec->nodes_num; node_i ++) {

        struct hda_widget* widget = hda_determine_widget(dev, hcodec, codec, node_i, 0);
        hcodec->widgets[node_i] = widget;
    }

    return hcodec;
}

char dat[] = {234, 149, 154, 56, 143, 34, 45, 78, 89, 123, 67};

struct hda_widget* hda_determine_widget(struct hda_dev* dev, struct hda_codec* cd, int codec, int node, int nest)
{
    struct hda_widget* widget = kmalloc(sizeof(struct hda_widget));

    widget->func_group_type = hda_codec_get_param(dev, codec, node, HDA_CODEC_FUNCTION_GROUP_TYPE);
    widget->caps = hda_codec_get_param(dev, codec, node, HDA_CODEC_PARAM_AUDIO_WIDGET_CAPS);
    widget->type = (widget->caps >> 20) & 0xF;

    uint32_t amp_cap = hda_codec_get_param(dev, codec, node, HDA_CODEC_AMP_CAPS);
    uint32_t subnodes = hda_codec_get_param(dev, codec, node, HDA_CODEC_PARAM_SUB_NODE_COUNT);

    uint32_t starting_node = (subnodes >> 16) & 0xFF;
    uint32_t nodes_num = (subnodes) & 0xFF;

    // Включить
    hda_codec_exec(dev, codec, node, HDA_VERB_SET_POWER_STATE, 0x0000);

    printk("node %i\n", node);

    for (int i = 0; i < nest; i ++) {
        printk("\t");
    }

    switch (widget->type) {
        case HDA_WIDGET_AUDIO_PIN_COMPLEX:
            
            printk("Widget type : PIN \n");

            widget->conn_defaults = hda_codec_exec(dev, codec, node, HDA_VERB_GET_CONNECTION_DEFAULTS, 0);
            //printk("default DEV %i\n", (widget->conn_defaults >> 20) & 0xF);

            // Включить пин виджет
            hda_codec_exec(dev, codec, node, HDA_VERB_SET_PIN_WIDGET_CTL, 0xC0);

            // Ищем все аудио входы/выходы
            uint32_t conn_list_len_value = hda_codec_get_param(dev, codec, node, HDA_CODEC_GET_CONN_LIST_LEN);
            int conn_list_len = conn_list_len_value & 0x07;
            int is_long_form = (conn_list_len_value & 0x80) != 0;

            // Обычно для 0 возвращаются значения под индексами 0, 1, 2, 3
            // Для 4 - под индексами 4, 5, 6, 7 и т.д.
            // Для Long Form все делится на 2
            int multiplier = is_long_form ? 2 : 4;

            for (int i = 0; i * multiplier < conn_list_len; i += multiplier) {

                uint32_t value = hda_codec_exec(dev, codec, node, HDA_VERB_GET_CONN_LIST_ENTRY, i);

                for (int conn_i = 0; conn_i < multiplier && (i + conn_i < conn_list_len); conn_i ++) {
                    int conn_node = value >> (8 * conn_i);

                    printk("    Connection node %i\n", conn_node);
                    widget->connections[widget->connections_num] = cd->widgets[conn_node];

                }
            }

            break;
        case HDA_WIDGET_AUDIO_OUTPUT:
            printk("Widget type : Audio output\n");

            widget->stream_formats = hda_codec_get_param(dev, codec, node, HDA_CODEC_SUPPORTED_STREAM_FORMATS);
            widget->pcm_rates = hda_codec_get_param(dev, codec, node, HDA_CODEC_PARAM_PCM_SIZE_RATE);

            //printk("Formats %i Rates %i \n", widget->stream_formats, widget->pcm_rates);

            uint16_t format = SR_44_KHZ | BITS_16 | 1;
            int stream_id = 1;
            widget->ostream = hda_device_init_stream(dev, stream_id, 2, format);

            hda_codec_send_verb(dev, hda_make_verb(codec, node, (HDA_VERB_SET_STREAM_FORMAT << 8) | format), NULL);
            hda_codec_send_verb(dev, hda_make_verb(codec, node, (HDA_VERB_SET_STREAM_ID << 8) | (stream_id << 4)), NULL);

            uint32_t amp = hda_codec_exec(dev, codec, node, HDA_VERB_GET_AMP_GAIN_MUTE, 0) | 0xb000 | 127;
            hda_codec_exec(dev, codec, node, HDA_VERB_SET_AMP_GAIN_MUTE, amp);

            hda_outb(dev, SSYNC, stream_id);

            hda_stream_write(widget->ostream, dat, sizeof(dat));

            printk("PRE");

            hda_stream_run(dev, widget->ostream);

            printk("AFTER\n");

            break;
        case HDA_WIDGET_AUDIO_INPUT:
            printk("Widget type : Audio input\n");
            break;
        default:
            printk("unknown widget!!!!");
            break;
    }

    for (int node_i = starting_node; node_i < starting_node + nodes_num; node_i ++) {

        struct hda_widget* widget = hda_determine_widget(dev, cd, codec, node_i, nest + 1);
        cd->widgets[node_i] = widget;
    }

    return widget;
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

uint32_t hda_codec_exec(struct hda_dev* dev, int codec, uint32_t node, uint32_t verb, uint32_t param)
{
    uint64_t out = 0;
    verb = hda_make_verb(codec, node, (verb << 8) | param);
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

    //printk("OLD %i %i %i %i\n", corbwp, hda_inw(dev, CORBWP), hda_inw(dev, RIRBWP), hda_inw(dev, CORBRP));

    dev->corb[corbwp] = verb;
    hda_outw(dev, CORBWP, corbwp);

    while (hda_inw(dev, RIRBWP) == rirbwp);

    if (out) {
        *out = dev->rirb[(rirbwp + 1) % dev->rirb_entries];
    }

    //printk("NEW %i %i %i\n", hda_inw(dev, CORBWP), hda_inw(dev, RIRBWP), hda_inw(dev, CORBRP));

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