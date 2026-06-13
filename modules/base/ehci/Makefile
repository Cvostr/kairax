CINCL=-isystem ../../../sdk/kxmdk -isystem ../../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector -Wno-error=int-conversion -Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration
CFLAGS=-c $(CWARN) $(CINCL) -mno-red-zone
COMPILER=gcc

all: ehci.ko

ehci.ko: usb_ehci.c
	$(COMPILER) $(CFLAGS) usb_ehci.c -o ehci.ko

clean:
	find . -name "*.ko" -type f -delete
	find . -name "*.o" -type f -delete