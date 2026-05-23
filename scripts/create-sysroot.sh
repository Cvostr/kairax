#!/bin/bash

if [ ! -d $KAIRAX_SYSROOT ]; then
    echo "Creating sysroot in $KAIRAX_SYSROOT"
    mkdir -p $KAIRAX_SYSROOT

    mkdir $KAIRAX_SYSROOT/bin
    mkdir $KAIRAX_SYSROOT/libs
    mkdir $KAIRAX_SYSROOT/dev
    mkdir $KAIRAX_SYSROOT/proc
    mkdir $KAIRAX_SYSROOT/tmp
    mkdir -p $KAIRAX_SYSROOT/usr/include
    mkdir -p $KAIRAX_SYSROOT/usr/lib
fi