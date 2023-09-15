ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -fPIC
COMPILER=gcc

all: libc_static libc errno

errno: errno.o
	ar rcs liberrno.a errno.o

libc: math.o stdlib.o string.o sys_files.o process.o stdio.o ../sys/syscalls.o
	ld -melf_x86_64 -shared -o libc_dyn.so math.o stdlib.o string.o sys_files.o stdio.o process.o ../sys/syscalls.o

libc_static: math.o stdlib.o string.o sys_files.o stdio.o process.o errno.o
	ar rcs libc.a math.o stdlib.o string.o sys_files.o stdio.o process.o errno.o

clean:
	rm -rf *.o libc.so

stdlib.o: stdlib.c
	$(COMPILER) $(ARGS) stdlib.c -o stdlib.o

string.o: string.c
	$(COMPILER) $(ARGS) string.c -o string.o

math.o: math.c
	$(COMPILER) $(ARGS) math.c -o math.o

stdio.o: stdio.c
	$(COMPILER) $(ARGS) stdio.c -o stdio.o

sys_files.o: sys_files.c
	$(COMPILER) $(ARGS) sys_files.c -o sys_files.o

process.o: process.c
	$(COMPILER) $(ARGS) process.c -o process.o

errno.o: errno.c
	$(COMPILER) $(ARGS) errno.c -o errno.o