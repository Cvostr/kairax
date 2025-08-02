CINCL=-isystem ../../sdk/kxmdk -isystem ../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) 
COMPILER=gcc

all: e1000.o

e1000.o: e1000.c
	$(COMPILER) $(CFLAGS) e1000.c -o e1000.ko

clean:
	find . -name "*.ko" -type f -delete