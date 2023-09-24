CFLAGS=-nostdlib -m64 -nostdinc -ffreestanding -std=c99 -fPIC -I./include -I../sys
CC=gcc
SRC_DIR=src
OBJ_DIR=obj
COMPILED_OBJS = obj/math.o \
				obj/process.o \
				obj/stdio.o \
				obj/stdlib.o \
				obj/string.o \
				obj/unistd.o \
				obj/sys_files.o \
				obj/bsdmalloc.o

all: $(OBJ_DIR) libc_static libc errno

$(OBJ_DIR):
	mkdir -p $@

errno: obj/errno.o
	ar rcs liberrno.a obj/errno.o

libc: $(COMPILED_OBJS)
	$(CC) -fPIC -nostdlib -shared -o libc.so $(COMPILED_OBJS) ../sys/syscalls.o

libc_static: $(COMPILED_OBJS) obj/errno.o
	ar rcs libc_stat.a $(COMPILED_OBJS) obj/errno.o

clean:
	rm -rf obj/*.o *.so *.a

obj/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@