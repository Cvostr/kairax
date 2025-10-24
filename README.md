# KAIRAX

Написанное с нуля миниатюрное монолитное ядро, частично POSIX совместимое. Большая часть написана на языке C. Сейчас поддерживается только архитектура x86-64. Может  подойти для изучения внутренних принципов работы ОС


# Ключевые возможности

 - Запуск с multiboot2
 - Вытесняющая многозадачность
 - Запуск пользовательских программ формата ELF
 - Базовая поддержка файловой системы ext2, FAT только для чтения
 - Базовая поддержка ahci, nvme
 - Начальная поддержка сети (IPv4 - TCP, UDP, ICMP)
 - Сетевые драйверы для rtl8139, e1000, USB Ethernet
 - Возможность загрузки модулей ядра
 - Начальная поддержка многоядерности
 - Обработчики сигналов в программах
 - Динамический линковщик с поддержкой разделяемых библиотек
 - Рабочие DOOM 1, Quake2

# Сборка
Для сборки необходим Linux и зависимости. Установка зависимостей для Ubuntu

```
sudo apt install  make build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo nasm qemu mtools wget unzip fuse libfuse-dev uuid-dev gcc binutils parted qemu-system-x86 xorriso grub-pc-bin
```

Дополнительно для сборки и запуска с aarch64

```
sudo apt install gcc-aarch64-linux-gnu qemu-system-arm
```

Для сборки ядра и образа достаточно выполнить
```
sh mkboot.sh
```

Для сборки SDK:
```
cd sdk/crt
make
cd ../libc
make
cd ../libkairax
make
```

Для сборки пользовательских программ
```
cd progs
sh build.sh
```

# Запуск
Уже подготовлены скрипты для запуска в Qemu на Linux и Windows
    ``run.sh, run.bat``
Скорее всего эти скрипты потребуют виртуальный диск. Его можно создать так 
    ```
    dd  if=/dev/zero  of=./drive.img  bs=250M  count=1
    ```