CINCL=-isystem ../../../sdk/kxmdk -isystem ../../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector -Wno-error=int-conversion -Wno-error=incompatible-pointer-types
CFLAGS=-c $(CWARN) $(CINCL) -mno-red-zone
COMPILER=gcc

all: usb_hub.o
	ld -r usb_hub.o -o usb_hub.ko

usb_hub.o: usb_hub.c
	$(COMPILER) $(CFLAGS) usb_hub.c -o usb_hub.o

clean:
	find . -name "*.ko" -type f -delete
	find . -name "*.o" -type f -delete