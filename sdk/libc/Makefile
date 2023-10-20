CFLAGS=-nostdlib -nostdinc -ffreestanding -std=c99 -fPIC -I./include -I../sys
CC=gcc
ASM=nasm
SRC_DIR=src
OBJ_DIRS =  obj \
			obj/stdio \
			obj/stdlib \
			obj/sys \
			obj/x64

COMPILED_OBJS = obj/math.o \
				obj/process.o \
				obj/stdio/stdfiles.o \
				obj/stdio/fopen.o \
				obj/stdio/fread.o \
				obj/stdio/printf.o \
				obj/stdio/printf_generic.o \
				obj/stdio/scanf.o \
				obj/stdio/scanf_generic.o \
				obj/stdlib/bsdmalloc.o \
				obj/stdlib/stdlib.o \
				obj/stdlib/abs.o \
				obj/stdlib/strtod.o \
				obj/stdlib/strtol.o \
				obj/stdlib/dtostr.o \
				obj/string.o \
				obj/strings.o \
				obj/unistd.o \
				obj/mman.o \
				obj/fcntl.o \
				obj/sys/stat.o \
				obj/sys/ioctl.o \
				obj/sys/wait.o \
				obj/ctype.o \
				obj/dirent.o \
				obj/x64/syscalls.o

all: $(OBJ_DIRS) libc errno

$(OBJ_DIRS):
	mkdir -p $@

errno: obj/errno.o
	ar rcs liberrno.a obj/errno.o

libc: $(COMPILED_OBJS)
	$(CC) -fPIC -nostdlib -shared -o libc.so $(COMPILED_OBJS)

clean:
	rm -rf obj/*.o *.so *.a

obj/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: $(SRC_DIR)/%.asm
	$(ASM) -felf64 $< -o $@