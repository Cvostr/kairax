#include "kairax/types.h"
#include "kairax/list/list.h"

#define GCAP 0x00
#define GCTL 0x08
#define WAKEEN      0x0C
#define STATESTS    0x0E
#define INTCTL      0x20
#define INTSTS      0x24

#define SSYNC       0x38
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

#define ISTREAM_REG_BASE 0x80

#define ISTREAM_CTLL    0x0
#define ISTREAM_CTLU    0x2
#define ISTREAM_STS     0x3
#define ISTREAM_LPIB    0x4
#define ISTREAM_CBL     0x8
#define ISTREAM_LVI     0xC
#define ISTREAM_FMT     0x12
#define ISTREAM_BDPL    0x18
#define ISTREAM_BDPU    0x1C

#define HDA_MAX_CODECS      15
#define HDA_MAX_WIDGETS     100
#define HDA_MAX_CONNECTIONS 20

#define HDA_STREAM_CTL_RST 0x1
#define HDA_STREAM_CTL_RUN 0x2
#define HDA_STREAM_CTL_IOC 0x4
#define HDA_STREAM_CTL_FEI 0x8
#define HDA_STREAM_CTL_DEI 0x10

#define HDAC_SDSTS_DESE  (1<<4)
#define HDAC_SDSTS_FIFOE (1<<3)
#define HDAC_SDSTS_BCIS  (1<<2)

#define	SR_48_KHZ   (0U)
#define	SR_44_KHZ   (1 << 14)
#define	BITS_32     (4 << 4)
#define	BITS_16     (1 << 4)
#define	BITS_8      (0)

struct HDA_BDL_ENTRY {
	uint64_t paddr;
	uint32_t length;
	uint32_t ioc;
} PACKED;

struct hda_stream {

    uint32_t                index;
    int                     type;

    uint32_t                reg_base;
    uint32_t                size;
    void*                   mem;

    uint32_t                bdl_num;
    struct HDA_BDL_ENTRY*   bdl;

    void*                   ep;
};

struct hda_widget {

    int type;
    int node;
    struct hda_stream* ostream;

    struct hda_dev* dev;

    // DACs, ADCs
    uint32_t connections_num;
    struct hda_widget* connections[HDA_MAX_CONNECTIONS];

    uint32_t conn_defaults;
    uint32_t func_group_type;
    uint32_t caps;
    uint32_t amp_cap;
    uint32_t stream_formats;
    uint32_t pcm_rates;
};

struct hda_codec {
    uint32_t codec;
    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t major;
    uint8_t minor;
    uint8_t revision;
    uint8_t stepping;

    uint8_t starting_node;
    uint8_t nodes_num;

    struct hda_widget* widgets[HDA_MAX_WIDGETS];
};

struct hda_dev {
    void* io_addr;

    int supports64;
    int iss_num;
    int oss_num;
    int bss_num;

    int ostream_reg_base;
    int bstream_reg_base;
    uint32_t streams_total;

    uint32_t* corb;
    uint64_t* rirb;

    int corb_entries;
    int rirb_entries;

    struct hda_codec* codecs[HDA_MAX_CODECS];

    // Все потоки в порядке расположения их с учетом типа
    struct hda_stream** streams;
    int ostreams;
};

#define GCTL_CRST 0b1

// -- VERBS
#define HDA_VERB_CODEC_GET_PARAM  0xF00
#define HDA_VERB_GET_CONN_LIST_ENTRY  0xF02
#define HDA_VERB_GET_STREAM_ID      0xF06
#define HDA_VERB_SET_STREAM_ID      0x706
#define HDA_VERB_SET_STREAM_FORMAT  0x200
#define HDA_VERB_SET_POWER_STATE    0x705
#define HDA_VERB_GET_AMP_GAIN_MUTE          0xB00
#define HDA_VERB_SET_AMP_GAIN_MUTE          0x300
#define HDA_VERB_GET_VOLUME_CONTROL         0xF0F
#define HDA_VERB_SET_VOLUME_CONTROL         0x70F
#define HDA_VERB_GET_PIN_WIDGET_CTL         0xF07
#define HDA_VERB_SET_PIN_WIDGET_CTL         0x707
#define HDA_VERB_GET_CONNECTION_DEFAULTS    0xF1C

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
#define HDA_CODEC_GET_CONN_LIST_LEN     0xE
#define HDA_CODEC_AMP_CAPS              0x12

// WIDGET TYPES
#define HDA_WIDGET_AUDIO_OUTPUT 0
#define HDA_WIDGET_AUDIO_INPUT 1
#define HDA_WIDGET_AUDIO_MIXER 2
#define HDA_WIDGET_AUDIO_SELECTOR 3
#define HDA_WIDGET_AUDIO_PIN_COMPLEX 4
#define HDA_WIDGET_AUDIO_POWER_WIDGET 5
#define HDA_WIDGET_AUDIO_VOLUME_KNOB 6
#define HDA_WIDGET_AUDIO_BEEP_GENERATOR 7

// DEFAULT DEVICE
#define HDA_LINE_OUT    0
#define HDA_SPEAKER     1
#define HDA_HP_OUT      2
#define HDA_CD          3
#define HDA_SPDIF_OUT   4
#define HDA_LINE_IN     8

// PIN CTL
#define PIN_CTL_H_PNH_ENABLE    (1 << 7)
#define PIN_CTL_OUT_ENABLE      (1 << 6)
#define PIN_CTL_IN_ENABLE       (1 << 5)

// PCM Formats
#define HDA_PCM_B32             (1 << 20)
#define HDA_PCM_B24             (1 << 19)
#define HDA_PCM_B20             (1 << 18)
#define HDA_PCM_B16             (1 << 17)
#define HDA_PCM_B8              (1 << 16)

#define HDA_PCM_8_0             (1)
#define HDA_PCM_11_025          (1 << 1)
#define HDA_PCM_16_0            (1 << 2)
#define HDA_PCM_22_05           (1 << 3)
#define HDA_PCM_32_0            (1 << 4)
#define HDA_PCM_44_1            (1 << 5)
#define HDA_PCM_48_0            (1 << 6)
#define HDA_PCM_88_2            (1 << 7)

int hda_set_volume(struct audio_endpoint* ep, uint8_t left, uint8_t right);

uint32_t hda_stream_write(struct hda_stream* stream, char* mem, uint32_t offset, uint32_t size);
void hda_stream_run(struct hda_dev* dev, struct hda_stream* stream);
void hda_register_stream(struct hda_dev* dev, struct hda_stream* stream);

void hda_int_handler(void* regs, struct hda_dev* data);
uint32_t hda_codec_exec(struct hda_dev* dev, int codec, uint32_t node, uint32_t verb, uint32_t param);
uint32_t hda_codec_get_param(struct hda_dev* dev, int codec, uint32_t node, uint32_t param);
int hda_codec_send_verb(struct hda_dev* dev, uint32_t verb, uint64_t* out);

struct hda_codec* hda_determine_codec(struct hda_dev* dev, int codec);
struct hda_widget* hda_determine_widget(struct hda_dev* dev, struct hda_codec* cd, int codec, int node, int nest);
struct hda_widget* hda_codec_get_widget(struct hda_codec* codec, int node_num);

int hda_controller_reset(struct hda_dev* dev);

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