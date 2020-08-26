#!/bin/sh -exu
srcdir=$(dirname $(readlink -f $0))

cross_compile() {
    target=$1
    builddir=/tmp/mandelbrot_$target
    rm -rf $builddir
    mkdir -p $builddir
    cd $builddir
    cmake -DCMAKE_TOOLCHAIN_FILE=${srcdir}/${target}.toolchain -DCMAKE_BUILD_TYPE=Release ${srcdir}
    make
    zip -j mandelbrot-${target}.zip mandelbrot.exe /usr/${target}/bin/SDL2.dll
}

cross_compile i686-w64-mingw32
cross_compile x86_64-w64-mingw32
