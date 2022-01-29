
CFLAGS=-Wall -g -Og -Ideps/secp256k1/include -Ideps/libsodium/src/libsodium/include -Ideps
LDFLAGS=

SUBMODULES=deps/libsodium deps/secp256k1

ARS=deps/secp256k1/.libs/libsecp256k1.a deps/libsodium/src/libsodium/.libs/libsodium.a
OBJS=sha256.o hkdf.o hmac.o sha512.o lnsocket.o error.o handshake.o crypto.o bigsize.o
DEPS=$(OBJS) $(ARS) config.h

all: test lnrpc

deps/libsodium/.git:
	@tools/refresh-submodules.sh $(SUBMODULES)

deps/secp256k1/.git:
	@tools/refresh-submodules.sh $(SUBMODULES)

config.h: configurator
	./configurator > $@

configurator: configurator.c
	$(CC) $< -o $@

%.o: %.c config.h $(ARS)
	@echo "cc $<"
	@$(CC) $(CFLAGS) -c $< -o $@

deps/secp256k1/src/libsecp256k1-config.h: deps/secp256k1/configure
	cd deps/secp256k1; \
	./configure --enable-module-ecdh

deps/libsodium/config.status: deps/libsodium/configure
	cd deps/libsodium; \
	./configure --enable-minimal

deps/secp256k1/configure: deps/secp256k1/.git
	cd deps/secp256k1; \
	./autogen.sh

deps/libsodium/configure: deps/libsodium/.git
	cd deps/libsodium; \
	./autogen.sh

deps/secp256k1/.libs/libsecp256k1.a: deps/secp256k1/src/libsecp256k1-config.h
	cd deps/secp256k1; \
	make -j2 libsecp256k1.la

deps/libsodium/src/libsodium/.libs/libsodium.a: deps/libsodium/config.status
	cd deps/libsodium/src/libsodium; \
	make -j2 libsodium.la

test: test.o $(DEPS)
	@echo "ld test"
	@$(CC) $(CFLAGS) test.o $(OBJS) $(ARS) $(LDFLAGS) -o $@

lnrpc: rpc.o commando.o $(DEPS)
	@echo "ld lnrpc"
	@$(CC) $(CFLAGS) rpc.o commando.o $(OBJS) $(ARS) $(LDFLAGS) -o $@

tags: fake
	find . -name '*.c' -or -name '*.h' | xargs ctags

clean: fake
	rm -f test lnrpc config.h $(OBJS)

distclean: clean
	rm -f $(ARS) deps/secp256k1/src/libsecp256k1-config.h
	cd deps/secp256k1; \
	make clean
	cd deps/libsodium; \
	make clean


.PHONY: fake
