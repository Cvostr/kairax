#!/bin/bash

set -e

BINUTILS_BUILD_DIR="binutils-build"
BINUTILS_DIR="binutils"
BINUTILS_BRANCH="binutils-2_40-branch-kairax"
BINUTILS_REPO="https://github.com/Cvostr/binutils-kairax.git"

export KAIRAX_SYSROOT=~/kairax1/root
export KAIRAX_TOOLCHAIN=~/kairax1/toolchain
export KAIRAX_PREFIX=~/kairax1/prefix

# Создадим корневую директорию для сборки toolchain, если отсутствует
if [ ! -d $KAIRAX_TOOLCHAIN ]; then
    mkdir -p $KAIRAX_TOOLCHAIN
fi

cd $KAIRAX_TOOLCHAIN

# Загрузим и соберем binutils, если отсутствует
if [ ! -d $BINUTILS_DIR ]; then
	# клонирование репозитория
	git clone -b $BINUTILS_BRANCH $BINUTILS_REPO $BINUTILS_DIR
fi

# Очистка и создание директории для кэша сборки
#rm -rf $BINUTILS_BUILD_DIR

if [ ! -d $BINUTILS_BUILD_DIR ]; then
    mkdir $BINUTILS_BUILD_DIR
fi

cd $BINUTILS_BUILD_DIR

echo "Configuring binutils"

"../$BINUTILS_DIR/configure" \
	--target="x86_64-pc-kairax" \
	--prefix=$KAIRAX_PREFIX \
	--with-sysroot=$KAIRAX_SYSROOT \
	--enable-initfini-array \
	--enable-shared \
	--enable-lto \
	--disable-nls \
	--disable-werror

echo "Building binutils"
make -j2
make install-strip
