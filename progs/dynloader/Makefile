CFLAGS=-nostdlib -m64 -c -nostdinc -ffreestanding -fPIC -I./include -I./ -I../../sdk/libc/include -I../../sdk/sys/
LDARGS=-melf_x86_64 -z noexecstack -pie
CC=gcc
ASM=nasm
SRC_DIR=src
COMPILED_OBJS = obj/entry.o \
				obj/loader.o \
				obj/linker.o \
				obj/elf.o \
				obj/string.o

all: $(COMPILED_OBJS)
	ld $(LDARGS) -o loader.elf $(COMPILED_OBJS) ../../sdk/sys/syscalls.o

obj/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

obj/%.o: $(SRC_DIR)/%.asm
	$(ASM) -felf64 $< -o $@