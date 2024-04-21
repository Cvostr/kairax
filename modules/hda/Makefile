CINCL=-isystem ../../sdk/kxmdk -isystem ../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) 
COMPILER=gcc

all: hda.o

hda.o: hda.c
	$(COMPILER) $(CFLAGS) hda.c -o hda.ko

clean:
	find . -name "*.ko" -type f -delete