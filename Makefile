
CFLAGS=-Wall -g -Og -Ideps/secp256k1/include -Ideps/libsodium/src/libsodium/include -Ideps

LDFLAGS=

ARS=deps/secp256k1/.libs/libsecp256k1.a deps/libsodium/src/libsodium/.libs/libsodium.a

all: netln

deps/secp256k1/src/libsecp256k1-config.h: deps/secp256k1/configure
	./configure

deps/secp256k1/configure:
	cd deps/secp256k1; \
	./autogen.sh

deps/libsodium/configure:
	cd deps/libsodium; \
	./autogen.sh

deps/secp256k1/.libs/libsecp256k1.a: deps/secp256k1/configure
	cd deps/secp256k1; \
	make -j2 libsecp256k1.la


deps/libsodium/src/libsodium/.libs/libsodium.a: deps/libsodium/configure
	cd deps/libsodium/src/libsodium; \
	make -j2 libsodium.la

netln: netln.c $(ARS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

tags: fake
	find . -name '*.c' -or -name '*.h' | xargs ctags

clean: fake
	rm -f netln
	cd deps/secp256k1; \
	make clean
	cd deps/libsodium; \
	make clean


.PHONY: fake
