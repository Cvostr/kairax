CINCL=-isystem ../../../sdk/kxmdk -isystem ../../../kernel -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector
CFLAGS=-c $(CWARN) $(CINCL) -mno-red-zone
COMPILER=gcc

all: cdc_eth.o

cdc_eth.o: cdc_eth.c
	$(COMPILER) $(CFLAGS) cdc_eth.c -o cdc_eth.ko

clean:
	find . -name "*.ko" -type f -delete