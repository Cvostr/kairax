CFLAGS=-nostdlib -nostdinc -ffreestanding -std=c99 -fPIC -I./include -I../sys
CC=gcc
SRC_DIR=src
OBJ_DIRS =  obj \
			obj/stdio

COMPILED_OBJS = obj/math.o \
				obj/process.o \
				obj/stdio.o \
				obj/stdio/fopen.o \
				obj/stdio/fread.o \
				obj/stdlib.o \
				obj/string.o \
				obj/unistd.o \
				obj/mman.o \
				obj/fcntl.o \
				obj/stat.o \
				obj/ioctl.o \
				obj/ctype.o \
				obj/dirent.o \
				obj/bsdmalloc.o

all: $(OBJ_DIRS) libc errno

$(OBJ_DIRS):
	mkdir -p $@

errno: obj/errno.o
	ar rcs liberrno.a obj/errno.o

libc: $(COMPILED_OBJS)
	$(CC) -fPIC -nostdlib -shared -o libc.so $(COMPILED_OBJS) ../sys/syscalls.o

clean:
	rm -rf obj/*.o *.so *.a

obj/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@