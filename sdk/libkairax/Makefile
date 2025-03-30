CFLAGS=-nostdlib -nostdinc -ffreestanding -std=c99 -fPIC -I../libc/include -I./include -I../sys
CC=gcc
ASM=nasm
SRC_DIR=src
OBJ_DIR=obj \
		obj/x64
COMPILED_OBJS = obj/input.o \
				obj/desktop.o \
				obj/net.o \
				obj/routectl.o \
				obj/kernel.o \
				obj/sysinfo.o \
				obj/x64/syscalls.o

all: $(OBJ_DIR) libkairax

$(OBJ_DIR):
	mkdir -p $@

libkairax: $(COMPILED_OBJS)
	$(CC) -fPIC -nostdlib -shared -o libkairax.so $(COMPILED_OBJS)

clean:
	rm -rf obj/*.o *.so *.a obj/x64/*.o

obj/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: $(SRC_DIR)/%.asm
	$(ASM) -felf64 $< -o $@