#!/bin/bash

set -e

export KAIRAX_PREFIX=~/kairax1/prefix
export KAIRAX_SYSROOT=~/kairax1/root
export KAIRAX_TOOLCHAIN=~/kairax1/toolchain

create_sysroot () {
    ./create-sysroot.sh
}

build_binutils () {
    sh ./binutils.sh
}

build_gcc () {
    sh ./gcc.sh
}

case $1 in
	binutils)
		build_binutils
		;;
	gcc)
		build_gcc
		;;
	sysroot)
		create_sysroot
		;;
	all)
        create_sysroot
		build_binutils
        build_gcc
		;;
	*)
		echo "Unknown $1"
		;;
esac