CINCL=-isystem ../../../sdk/kxmdk -isystem ../../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) 
COMPILER=gcc

all: nvme.o nvme_ns.o
	ld -r nvme.o nvme_ns.o -o nvme.ko

nvme.o: nvme.c
	$(COMPILER) $(CFLAGS) nvme.c -o nvme.o

nvme_ns.o: nvme_ns.c
	$(COMPILER) $(CFLAGS) nvme_ns.c -o nvme_ns.o

clean:
	find . -name "*.ko" -type f -delete
	find . -name "*.o" -type f -delete