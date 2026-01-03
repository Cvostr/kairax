CINCL=-isystem ../../../sdk/kxmdk -isystem ../../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) -mno-red-zone
COMPILER=gcc

all: usb_mass.ko

usb_mass.ko: usb_mass_storage.c
	$(COMPILER) $(CFLAGS) usb_mass_storage.c -o usb_mass.ko -D__LITTLE_ENDIAN__

clean:
	find . -name "*.ko" -type f -delete