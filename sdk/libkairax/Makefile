CFLAGS=-nostdlib -m64 -c -nostdinc -ffreestanding -fPIC -I./include -I../sys
COMPILER=gcc
SRC_DIR=src
OBJ_DIR=obj
COMPILED_OBJS = obj/input.o \
				obj/desktop.o

all: $(OBJ_DIR) libkairax

$(OBJ_DIR):
	mkdir -p $@

libkairax: $(COMPILED_OBJS)
	ld -melf_x86_64 -shared -o libkairax.so $(COMPILED_OBJS) ../sys/syscalls.o

clean:
	rm -rf obj/*.o *.so *.a

obj/%.o: $(SRC_DIR)/%.c
	$(COMPILER) $(CFLAGS) $< -o $@