#include "kairax/types.h"

struct hda_dev {
    void* io_addr;

    int supports64;
    int iss_num;
    int oss_num;
    int bss_num;

    int ostream_reg_base;

    uint32_t* corb;
    uint64_t* rirb;

    int corb_entries;
    int rirb_entries;

    void* istream;
    void* ostream;
};

#define GCAP 0x00
#define GCTL 0x08
#define WAKEEN      0x0C
#define STATESTS    0x0E
#define INTCTL 0x20

#define CORBLBASE   0x40
#define CORBUBASE   0x44
#define CORBWP      0x48
#define CORBRP      0x4A
#define CORBCTL     0x4C
#define CORBSIZE    0x4E

#define RIRBLBASE   0x50
#define RIRBUBASE   0x54
#define RIRBWP      0x58
#define RIRBCTL     0x5C
#define RIRBSTS     0x5D
#define RIRBSIZE    0x5E
#define RINTCNT     0x5A

#define DPLBASE     0x70
#define DPUBASE     0x74

#define ISTREAM_CTLL    0x80
#define ISTREAM_STS     0x83

#define OSTREAM_CTLL    0x100
#define OSTREAM_CTLU    0x102
#define OSTREAM_STS     0x103
#define OSTREAM_CBL     0x108 

#define HDA_STREAM_BUFFER_SIZE  2048
#define BDL_SIZE                4

#define HDA_MAX_CODECS 15

// -- VERBS
#define HDA_VERB_CODEC_GET_PARAM  0xF00
#define HDA_VERB_GET_CONNECTION_LIST_ENTRY  0xF02

#define HDA_VERB_GET_AMP_GAIN_MUTE  0xB
#define HDA_VERB_SET_AMP_GAIN_MUTE  0x3

// CODEC PARAMS
#define HDA_CODEC_PARAM_VENDOR_ID       0
#define HDA_CODEC_PARAM_REVISION_ID     0x2
#define HDA_CODEC_PARAM_SUB_NODE_COUNT  0x4
#define HDA_CODEC_FUNCTION_GROUP_TYPE   0x5
#define HDA_CODEC_FUNCTION_GROUP_CAPS   0x8
#define HDA_CODEC_PARAM_AUDIO_WIDGET_CAPS   0x9
#define HDA_CODEC_PARAM_PCM_SIZE_RATE   0xA
#define HDA_CODEC_SUPPORTED_STREAM_FORMATS  0xB
#define HDA_CODEC_PIN_CAPABILITIES      0xC

void hda_int_handler(void* regs, struct hda_dev* data);
uint32_t hda_codec_get_param(struct hda_dev* dev, int codec, uint32_t node, uint32_t param);
int hda_codec_send_verb(struct hda_dev* dev, uint32_t verb, uint64_t* out);

static inline void hda_outd(struct hda_dev* dev, uintptr_t base, uint32_t value) 
{
    uint32_t* addr = (uint32_t*) (dev->io_addr + base);
    *addr = value;
}

static inline uint32_t hda_ind(struct hda_dev* dev, uintptr_t base) 
{
    uint32_t* addr = (uint32_t*) (dev->io_addr + base);
    return *addr;
}

static inline void hda_outw(struct hda_dev* dev, uintptr_t base, uint16_t value) 
{
    uint16_t* addr = (uint16_t*) (dev->io_addr + base);
    *addr = value;
}

static inline uint16_t hda_inw(struct hda_dev* dev, uintptr_t base) 
{
    uint16_t* addr = (uint16_t*) (dev->io_addr + base);
    return *addr;
}

static inline void hda_outb(struct hda_dev* dev, uintptr_t base, uint8_t value) 
{
    uint8_t* addr = (uint8_t*) (dev->io_addr + base);
    *addr = value;
}

static inline uint8_t hda_inb(struct hda_dev* dev, uintptr_t base) 
{
    uint8_t* addr = (uint8_t*) (dev->io_addr + base);
    return *addr;
}

static inline void delay() {
    for (int i = 0; i < 10000000; i++);
}

static inline uint32_t hda_make_verb(int codec, int node_id, uint32_t payload) 
{
    return ((codec & 0xF) << 28) | ((node_id & 0xFF) << 20) | (payload & 0xFFFFF);
}