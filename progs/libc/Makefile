ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -fPIC
COMPILER=gcc

all: libc

libc: math.o stdlib.o string.o
	ld -melf_x86_64 -shared -o libc.so math.o stdlib.o string.o

stdlib.o: stdlib.c
	$(COMPILER) $(ARGS) stdlib.c -o stdlib.o

string.o: string.c
	$(COMPILER) $(ARGS) string.c -o string.o

math.o: math.c
	$(COMPILER) $(ARGS) math.c -o math.o