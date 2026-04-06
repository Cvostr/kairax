CINCL=-isystem ../../../sdk/kxmdk -isystem ../../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) -mno-red-zone
COMPILER=gcc

all: usb_xhci.o usb_xhci_device.o
	ld -r usb_xhci.o usb_xhci_device.o -o xhci.ko

usb_xhci.o: usb_xhci.c
	$(COMPILER) $(CFLAGS) usb_xhci.c -o usb_xhci.o

usb_xhci_device.o: usb_xhci_device.c
	$(COMPILER) $(CFLAGS) usb_xhci_device.c -o usb_xhci_device.o

clean:
	find . -name "*.ko" -type f -delete
	find . -name "*.o" -type f -delete