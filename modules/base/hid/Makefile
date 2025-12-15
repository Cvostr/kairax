CINCL=-isystem ../../../sdk/kxmdk -isystem ../../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) -mno-red-zone
COMPILER=gcc

all: usb_hid.o usb_hid_kbd.o usb_hid_mouse.o
	ld -r usb_hid.o usb_hid_kbd.o usb_hid_mouse.o -o usb_hid.ko

usb_hid.o: usb_hid.c
	$(COMPILER) $(CFLAGS) usb_hid.c -o usb_hid.o

usb_hid_kbd.o: usb_hid_kbd.c
	$(COMPILER) $(CFLAGS) usb_hid_kbd.c -o usb_hid_kbd.o

usb_hid_mouse.o: usb_hid_mouse.c
	$(COMPILER) $(CFLAGS) usb_hid_mouse.c -o usb_hid_mouse.o

clean:
	find . -name "*.ko" -type f -delete
	find . -name "*.o" -type f -delete