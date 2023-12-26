

struct hda_dev {
    void* io_addr;
};

#define CRST 0x08
#define INTCTL 0x20

void hda_outd(struct hda_dev* dev, uintptr_t base, uint32_t value);
uint32_t hda_ind(struct hda_dev* dev, uintptr_t base);