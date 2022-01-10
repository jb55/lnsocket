
CFLAGS=-Wall -g -Og -Ideps/secp256k1

LDFLAGS=

all: netln

deps/secp256k1/src/libsecp256k1-config.h: deps/secp256k1/configure
	./configure

deps/secp256k1/configure:
	cd deps/secp256k1; \
	./autogen.sh

deps/secp256k1/.libs/libsecp256k1.a: deps/secp256k1/configure
	cd deps/secp256k1; \
	make -j2 libsecp256k1.la

netln: netln.c deps/secp256k1/.libs/libsecp256k1.a
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean: fake
	rm -f netln
	cd deps/secp256k1; \
	make clean


.PHONY: fake
