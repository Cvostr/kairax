# KAIRAX

Написанное с нуля миниатюрное монолитное ядро, частично POSIX совместимое. Большая часть написана на языке C. Сейчас поддерживается только архитектура x86-64. Может  подойти для изучения внутренних принципов работы ОС


# Ключевые возможности

 - Запуск с multiboot2
 - Вытесняющая многозадачность
 - Запуск пользовательских программ формата ELF
 - Базовая поддержка файловой системы ext2
 - Базовая поддержка ahci, nvme
 - Начальная поддержка сети и драйвер rtl8139
 - Возможность загрузки модулей ядра
 - Начальная поддержка многоядерности

# Сборка
Для сборки необходим Linux и зависимости. Установка зависимостей для Ubuntu
``
sudo apt install  make build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo nasm qemu mtools wget unzip fuse libfuse-dev uuid-dev gcc binutils parted qemu-system-x86 xorriso grub-pc-bin
``
Для сборки ядра и образа достаточно выполнить
``sh mkboot.sh``
Для сборки SDK:
``cd sdk/crt``
``make``
``cd ../libc``
``make``
``cd ../libkairax``
``make``
Для сборки пользовательских программ
``cd progs``
``sh build.sh``

# Запуск
  
   Уже подготовлены скрипты для запуска в Qemu на Linux и Windows
    Linux
    ``run.sh, run.bat``
    Скорее всего эти скрипты потребуют виртуальный диск. Его можно создать так 
    ``dd  if=/dev/zero  of=./drive.img  bs=250M  count=1``