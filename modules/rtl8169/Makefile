CINCL=-isystem ../../sdk/kxmdk -isystem ../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) 
COMPILER=gcc

all: rtl8169.o

rtl8169.o: rtl8169.c
	$(COMPILER) $(CFLAGS) rtl8169.c -o rtl8169.ko

clean:
	find . -name "*.ko" -type f -delete