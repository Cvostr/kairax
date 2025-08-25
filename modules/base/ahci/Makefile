CINCL=-isystem ../../../sdk/kxmdk -isystem ../../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) -mno-red-zone
COMPILER=gcc

all: ahci.o ahci_port.o
	ld -r ahci.o ahci_port.o -o ahci.ko

ahci.o: ahci.c
	$(COMPILER) $(CFLAGS) ahci.c -o ahci.o

ahci_port.o: ahci_port.c
	$(COMPILER) $(CFLAGS) ahci_port.c -o ahci_port.o

clean:
	find . -name "*.ko" -type f -delete
	find . -name "*.o" -type f -delete