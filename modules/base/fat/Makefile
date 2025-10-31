CINCL=-isystem ../../../sdk/kxmdk -isystem ../../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) -mno-red-zone
COMPILER=gcc

all: fat.ko

fat.ko: fat.c
	$(COMPILER) $(CFLAGS) fat.c -o fat.ko

clean:
	find . -name "*.ko" -type f -delete
	find . -name "*.o" -type f -delete