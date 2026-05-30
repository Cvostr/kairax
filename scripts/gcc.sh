#!/bin/bash

set -e

GCC_DIR="gcc"
GCC_BUILD_DIR="gcc-build"
GCC_BRANCH="releases/gcc-13-kairax"
GCC_REPO="https://github.com/Cvostr/gcc-kairax.git"

# Создадим корневую директорию для сборки toolchain, если отсутствует
if [ ! -d $KAIRAX_TOOLCHAIN ]; then
    mkdir -p $KAIRAX_TOOLCHAIN
fi

cd $KAIRAX_TOOLCHAIN
if [ ! -d $GCC_DIR ]; then
	# клонирование репозитория
	git clone -b $GCC_BRANCH $GCC_REPO $GCC_DIR
fi

# Очистка и создание директории для кэша сборки
#rm -rf $GCC_BUILD_DIR
if [ ! -d $GCC_BUILD_DIR ]; then
    mkdir $GCC_BUILD_DIR
fi

cd $GCC_BUILD_DIR

echo "Configuring GCC"

"../$GCC_DIR/configure" \
	--target="x86_64-pc-kairax" \
	--prefix=$KAIRAX_PREFIX \
	--with-sysroot=$KAIRAX_SYSROOT \
	--enable-initfini-array \
	--enable-shared \
	--enable-lto \
	--disable-nls \
	--disable-werror \
	--enable-languages=c

echo "Building GCC"

make -j2
make install-strip