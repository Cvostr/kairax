CINCL=-isystem ../../sdk/kxmdk -isystem ../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) 
COMPILER=gcc

all: rtl8139.o

rtl8139.o: rtl8139.c
	$(COMPILER) $(CFLAGS) rtl8139.c -o rtl8139.ko

clean:
	find . -name "*.ko" -type f -delete