#!/usr/bin/env bash

cd deps/secp256k1

export CC=emcc
export AR=emar
export RANLIB=emranlib

export PREFIX="$(pwd)/libsecp256k1-wasm"

mkdir -p $PREFIX || exit 1

make distclean > /dev/null

./configure --disable-shared \
	    --disable-tests \
	    --disable-exhaustive-tests \
	    --disable-benchmark \
	    --enable-module-ecdh \
            --prefix="$PREFIX"

make -j3 install || exit 1

rm -rf -- "$PREFIX/tmp"
make distclean > /dev/null
