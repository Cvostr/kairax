ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding
COMPILER=gcc
ASM=nasm

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

all: entry.o

clean:
	rm -rf *.o

entry.o: entry.asm
	$(ASM) -felf64 entry.asm -o entry.o

install: entry.o
	mkdir -p $(BINDIR)
	cp entry.o $(BINDIR)/