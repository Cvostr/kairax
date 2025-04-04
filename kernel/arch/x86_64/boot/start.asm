bits 32

%include "memory/mem_layout.asm"

%define KERNEL_STACK_SIZE         (4096)
%define MAGIC_HEADER_MB2          0xE85250D6
%define MAGIC_BOOTLOADER_MB2      0x36d76289

extern gdtptr
extern kmain
extern init_x64
global kernel_stack_top
global p4_table
extern x64_sse_enable
extern x64_osxsave_enable
extern x64_avx_enable

global _start

section .text
_start:
	cli 							; Отключение прерываний
	mov esp, K2P(kernel_stack_top) 	; Назначение стековой памяти
	push ebx						; Сохранение указателя на таблицу multiboot2
	
	call check_multiboot2			;Проверка multiboot2
	call check_cpuid
	call check_x64

	call beginning_memmap
	call enable_long_mode_paging

	mov eax, K2P(gdtptr)
	lgdt [eax]

	mov edi, MAGIC_BOOTLOADER_MB2
	pop esi							;Вернуть структуру multiboot2 в регистр первого аргумента (EDI)

	jmp 0x08:K2P(init_x64)			;Выполнить дальний прыжок

	hlt


check_multiboot2:
	cmp eax, MAGIC_BOOTLOADER_MB2
	jne no_multiboot2
	ret
no_multiboot2:
	hlt

check_cpuid: ; проверка поддержки инструкции CPUID
	pushfd
	pop eax
	mov ecx, eax
	xor eax, 1 << 21
	push eax
	popfd
	pushfd
	pop eax
	
	push ecx
	popfd
	cmp eax, ecx
	je no_cpuid ; Ошибка, CPUID не поддерживается
	ret
no_cpuid:
	hlt

check_x64: ;Проверка, поддерживает ли процессор Long Mode
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_x64

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_x64
    ret
no_x64:
	hlt

beginning_memmap: ;Задает начальную конфигурацию для страничной памяти
	mov eax, K2P(p3_table) ; В eax помещается адрес таблицы 3 уровня
	or eax, 0b11 	; Память можно просматривать и изменять
	mov DWORD [K2P(p4_table)], eax
	mov DWORD [K2P(p4_table) + (PML4_ENTRY_INDEX_KERN * PAGE_ENTRY_SIZE)], eax

	mov eax, K2P(p3_table_1) ; В eax помещается адрес таблицы 3 уровня
	or eax, 0b11 	; Память можно просматривать и изменять
	mov DWORD [K2P(p4_table)], eax
	mov DWORD [K2P(p4_table) + (PML4_ENTRY_INDEX_PHYS * PAGE_ENTRY_SIZE)], eax

	mov eax, K2P(p2_table) ; В eax помещается адрес таблицы 2 уровня
	or eax, 0b11	; Память можно просматривать и изменять
	mov DWORD [K2P(p3_table)], eax
	mov DWORD [K2P(p3_table) + (PDPT_ENTRY_INDEX_KERN * PAGE_ENTRY_SIZE)], eax

	mov eax, K2P(p2_table) ; В eax помещается адрес таблицы 2 уровня
	or eax, 0b11	; Память можно просматривать и изменять
	mov DWORD [K2P(p3_table_1)], eax
	mov DWORD [K2P(p3_table_1) + (PDPT_ENTRY_INDEX_PHYS * PAGE_ENTRY_SIZE)], eax

	xor ecx, ecx 	; Быстрое обнуление счетчика
.p1_loop:
	mov eax, 0x200000 	;размер фреймов таблицы - 2 мб
	mul ecx				;умножаем размер на номер 2-мб страницы
	or eax, 0b10000011 ; Память можно просматривать и изменять, а также это очень большая страница
	mov [K2P(p2_table) + ecx * 8], eax ; map ecx-th entry

	inc ecx 		; увеличение счетчика на 1
	cmp ecx, 512 	; уже создано 512 страниц
	jne .p1_loop
	ret

enable_long_mode_paging: ;включение 64 битного режима процессора и страничной адресации
	mov eax, K2P(p4_table)
	mov cr3, eax
	;Взведение флага PAE, PGE в регистре cr4
	mov eax, cr4
	or eax, (1 << 5) | (1 << 7)
	mov cr4, eax
	;
	mov ecx, 0xC0000080
	rdmsr
	or eax, (1 << 8) | (0x1 << 0xB) | (1 << 11) | 1 ; Включение Long Mode, вызова syscall, SVME
	wrmsr
	; Включение страничной адресации
	mov eax, cr0
	or eax, (1 << 31) | (1 << 0) | (1 << 16)
	mov cr0, eax
	ret
	
global p4_table

section .bss
align 0x1000
p4_table:
    resb 0x1000
p3_table:
    resb 0x1000
p3_table_1:
    resb 0x1000
p2_table:
    resb 0x1000
; Стековая память
kernel_stack_bottom:
    resb KERNEL_STACK_SIZE
kernel_stack_top: