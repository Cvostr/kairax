ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -fPIC
COMPILER=gcc

all: libc_static

libc: math.o stdlib.o string.o
	ld -melf_x86_64 -shared -o libc.so math.o stdlib.o string.o sys_files.o

libc_static: math.o stdlib.o string.o sys_files.o stdio.o
	ar rcs libc.a math.o stdlib.o string.o sys_files.o stdio.o

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