#!/bin/sh -ex
mkdir -p build.gcc
cd build.gcc
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
./benchmark
cd ..

mkdir -p build.clang
cd build.clang
CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
./benchmark
cd ..
