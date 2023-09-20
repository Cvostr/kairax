CFLAGS=-nostdlib -m64 -c -nostdinc -ffreestanding -fPIC -I./include -I../sys
COMPILER=gcc
SRC_DIR=src
OBJ_DIR=obj
COMPILED_OBJS = obj/math.o \
				obj/process.o \
				obj/stdio.o \
				obj/stdlib.o \
				obj/string.o \
				obj/sys_files.o \
				obj/bsdmalloc.o

all: libc_static libc errno

errno: obj/errno.o
	ar rcs liberrno.a obj/errno.o

libc: $(COMPILED_OBJS)
	ld -melf_x86_64 -shared -o libc.so $(COMPILED_OBJS) ../sys/syscalls.o

libc_static: $(COMPILED_OBJS) obj/errno.o
	ar rcs libc_stat.a $(COMPILED_OBJS) obj/errno.o

clean:
	rm -rf obj/*.o *.so *.a

obj/%.o: $(SRC_DIR)/%.c
	$(COMPILER) $(CFLAGS) $< -o $@