#!/usr/bin/env bash



git submodule update --init --remote

cd fibril

./bootstrap
mkdir -p build
./configure --prefix=$(pwd)/build

make
make install

cd -


./bootstrap
mkdir -p build
./configure --prefix=$(pwd)/build

make
make install



exit 0
