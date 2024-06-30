ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include -I../../sdk/libkairax/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
#ARGS=-m64 -c
COMPILER=gcc

all: main.o lexer.o ast.o ast_executor.o list.o
#	gcc -o rxsh.a main.o lexer.o list.o ast_executor.o ast.o	
	ld $(LDARGS) -o rxsh.a main.o lexer.o list.o ast_executor.o ast.o ../../sdk/crt/entry.o -L../../sdk/libc/ -L../../sdk/libkairax/ -lc -lerrno -lkairax

clean:
	rm -rf *.o *.a

main.o: main.c
	$(COMPILER) $(ARGS) main.c -o main.o

lexer.o: lexer.c
	$(COMPILER) $(ARGS) lexer.c -o lexer.o

ast.o: ast.c
	$(COMPILER) $(ARGS) ast.c -o ast.o

ast_executor.o: ast_executor.c
	$(COMPILER) $(ARGS) ast_executor.c -o ast_executor.o

list.o: list.c
	$(COMPILER) $(ARGS) list.c -o list.o