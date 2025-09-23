CFLAGS=-nostdlib -nostdinc -ffreestanding -D__LITTLE_ENDIAN__ -std=c99 -fPIC -I./include -I../sys
CC=gcc
ASM=nasm
SRC_DIR=src
OBJ_DIRS =  obj \
			obj/stdio \
			obj/stdlib \
			obj/sys \
			obj/threads \
			obj/netinet \
			obj/arpa \
			obj/x64

COMPILED_OBJS = obj/math.o \
				obj/process.o \
				obj/stdio/stdfiles.o \
				obj/stdio/fopen.o \
				obj/stdio/fread.o \
				obj/stdio/fwrite.o \
				obj/stdio/fops.o \
				obj/stdio/fflush.o \
				obj/stdio/printf.o \
				obj/stdio/sprintf.o \
				obj/stdio/fdprintf.o \
				obj/stdio/fprintf.o \
				obj/stdio/printf_generic.o \
				obj/stdio/scanf.o \
				obj/stdio/scanf_generic.o \
				obj/stdio/put.o \
				obj/stdio/get.o \
				obj/stdio/fgets.o \
				obj/stdio/perror.o \
				obj/stdlib/bsdmalloc.o \
				obj/stdlib/stdlib.o \
				obj/stdlib/abs.o \
				obj/stdlib/strtod.o \
				obj/stdlib/strtol.o \
				obj/stdlib/dtostr.o \
				obj/stdlib/rand.o \
				obj/stdlib/abort.o \
				obj/stdlib/system.o \
				obj/stdlib/env.o \
				obj/stdlib/putenv.o \
				obj/stdlib/atexit.o \
				obj/stdlib/qsort.o \
				obj/netinet/in.o \
				obj/arpa/inet.o \
				obj/arpa/nameser.o \
				obj/string.o \
				obj/strtok.o \
				obj/strings.o \
				obj/unistd.o \
				obj/hostname.o \
				obj/exec.o \
				obj/assert.o \
				obj/signal.o \
				obj/sys/mman.o \
				obj/fcntl.o \
				obj/sys/stat.o \
				obj/sys/ioctl.o \
				obj/sys/wait.o \
				obj/sys/reboot.o \
				obj/sys/futex.o \
				obj/sys/socket.o \
				obj/sys/mount.o \
				obj/time.o \
				obj/gmtime_r.o \
				obj/timegm.o \
				obj/ctype.o \
				obj/dirent.o \
				obj/spawn.o \
				obj/sched.o \
				obj/termios.o \
				obj/netdb.o \
				obj/threads/mtx.o \
				obj/threads/cnd.o \
				obj/x64/syscalls.o \
				obj/x64/math.o \
				obj/x64/setjmp.o

all: $(OBJ_DIRS) libc errno

$(OBJ_DIRS):
	mkdir -p $@

errno: obj/errno.o
	ar rcs liberrno.a obj/errno.o

libc: $(COMPILED_OBJS)
	$(CC) -fPIC -nostdlib -shared -o libc.so $(COMPILED_OBJS)

clean:
	find . -name "*.o" -type f -delete

obj/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: $(SRC_DIR)/%.asm
	$(ASM) -felf64 $< -o $@