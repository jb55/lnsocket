#!/usr/bin/env bash

cd deps/libsodium

export CC=emcc
export AR=emar

export PREFIX="$(pwd)/libsodium-wasm"

mkdir -p $PREFIX || exit 1

make distclean > /dev/null

./configure --disable-shared --enable-minimal --disable-ssp --prefix="$PREFIX"

make -j3 install || exit 1

make distclean > /dev/null
