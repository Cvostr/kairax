CINCL=-isystem ../../sdk/kxmdk -DMODULE
CWARN=-W -Wall -Wstrict-prototypes -Wmissing-prototypes
CFLAGS=-c $(CWARN) $(CINCL) 
COMPILER=gcc

all: module.o

module.o: module.c
	$(COMPILER) $(CFLAGS) module.c -o module.ko