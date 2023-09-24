CFLAGS=-nostdlib -nostdinc -ffreestanding -I./include -I../sys -mcmodel=large
COMPILER=gcc
SRC_DIR=src
OBJ_DIR=obj
COMPILED_OBJS = obj/input.o \
				obj/desktop.o

all: $(OBJ_DIR) libkairax

$(OBJ_DIR):
	mkdir -p $@

libkairax: $(COMPILED_OBJS)
	$(CC) -nostdlib -mcmodel=large -shared -o libkairax.so $(COMPILED_OBJS)

clean:
	rm -rf obj/*.o *.so *.a

obj/%.o: $(SRC_DIR)/%.c
	$(COMPILER) $(CFLAGS) -c $< -o $@