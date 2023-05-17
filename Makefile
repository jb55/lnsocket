
CFLAGS=-Wall -Os -Ideps/secp256k1/include -Ideps/libsodium/src/libsodium/include -Ideps
LDFLAGS=

SUBMODULES=deps/secp256k1

# Build for the simulator
XCODEDIR=$(shell xcode-select -p)
SIM_SDK=$(XCODEDIR)/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk
IOS_SDK=$(XCODEDIR)/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk

HEADERS=config.h deps/secp256k1/include/secp256k1.h deps/libsodium/src/libsodium/include/sodium/crypto_aead_chacha20poly1305.h
ARS=libsecp256k1.a libsodium.a lnsocket.a
WASM_ARS=target/js/libsecp256k1.a target/js/libsodium.a target/js/lnsocket.a
OBJS=sha256.o hkdf.o hmac.o sha512.o lnsocket.o error.o handshake.o crypto.o bigsize.o commando.o bech32.o
ARM64_OBJS=$(OBJS:.o=-arm64.o)
X86_64_OBJS=$(OBJS:.o=-x86_64.o)
WASM_OBJS=$(OBJS:.o=-wasm.o) lnsocket_wasm-wasm.o
BINS=ctest lnrpc

DEPS=$(OBJS) $(ARS) $(HEADERS)

all: $(BINS) $(ARS)

ios: target/ios/lnsocket.a target/ios/libsodium.a target/ios/libsecp256k1.a

js: target/js/lnsocket.js target/js/lnsocket.wasm
	@mkdir -p dist/js
	cp target/js/lnsocket.wasm target/js/lnsocket.js dist/js

node: target/node/lnsocket.js target/node/lnsocket.wasm
	@mkdir -p dist/node
	cp target/node/lnsocket.wasm target/node/lnsocket.js dist/node

liblnsocket.a: lnsocket.a
	cp lnsocket.a liblnsocket.a

rust: liblnsocket.a
	cargo build --release

target/node/lnsocket.js: target/tmp/node/lnsocket.js lnsocket_lib.js
	@mkdir -p target/node
	cat $^ > $@

target/js/lnsocket.js: target/tmp/js/lnsocket.js lnsocket_lib.js
	@mkdir -p target/js
	cat $^ > $@

libsodium-1.0.18-stable.tar.gz:
	curl -O https://download.libsodium.org/libsodium/releases/libsodium-1.0.18-stable.tar.gz

deps/libsodium/configure: libsodium-1.0.18-stable.tar.gz
	tar xvf $^; \
	mkdir -p deps; \
	mv libsodium-stable deps/libsodium

deps/secp256k1/.git:
	@tools/refresh-submodules.sh $(SUBMODULES)

lnsocket.a: $(OBJS)
	ar rcs $@ $(OBJS)

target/arm64/lnsocket.a: $(ARM64_OBJS)
	@mkdir -p target/arm64
	ar rcs $@ $^

target/x86_64/lnsocket.a: $(X86_64_OBJS)
	@mkdir -p target/x86_64
	ar rcs $@ $^

target/js/lnsocket.a: $(WASM_OBJS)
	@mkdir -p target/js
	emar rcs $@ $^

target/ios/lnsocket.a: target/x86_64/lnsocket.a target/arm64/lnsocket.a
	@mkdir -p target/ios
	lipo -create $^ -output $@

%-arm64.o: %.c $(HEADERS)
	@echo "cc $@"
	@$(CC) $(CFLAGS) -c $< -o $@ -arch arm64 -isysroot $(IOS_SDK) -target arm64-apple-ios -fembed-bitcode

%-wasm.o: %.c $(HEADERS)
	@echo "emcc $@"
	@emcc $(CFLAGS) -c $< -o $@

%-x86_64.o: %.c $(HEADERS)
	@echo "cc $@"
	@$(CC) $(CFLAGS) -c $< -o $@ -arch x86_64 -isysroot $(SIM_SDK) -mios-simulator-version-min=6.0.0 -target x86_64-apple-ios-simulator

# TODO cross compiled config??
config.h: configurator
	./configurator > $@

configurator: configurator.c
	$(CC) $< -o $@

%.o: %.c $(HEADERS)
	@echo "cc $@"
	@$(CC) $(CFLAGS) -c $< -o $@

deps/secp256k1/include/secp256k1.h: deps/secp256k1/.git

deps/libsodium/src/libsodium/include/sodium/crypto_aead_chacha20poly1305.h: deps/libsodium/configure

deps/secp256k1/config.log: deps/secp256k1/configure
	cd deps/secp256k1; \
	./configure --disable-shared --enable-module-ecdh --enable-module-extrakeys --enable-module-schnorrsig

deps/libsodium/config.status: deps/libsodium/configure
	cd deps/libsodium; \
	./configure --disable-shared --enable-minimal

deps/secp256k1/configure: deps/secp256k1/.git
	cd deps/secp256k1; \
	patch -p1 < ../../tools/0001-configure-customizable-AR-and-RANLIB.patch; \
	./autogen.sh

deps/libsodium/config.log: deps/libsodium/configure
	cd deps/libsodium; \
	./configure

deps/secp256k1/.libs/libsecp256k1.a: deps/secp256k1/config.log
	cd deps/secp256k1; \
	make -j libsecp256k1.la

libsecp256k1.a: deps/secp256k1/.libs/libsecp256k1.a
	cp $< $@

libsodium.a: deps/libsodium/src/libsodium/.libs/libsodium.a
	cp $< $@

target/ios/libsodium.a: deps/libsodium/libsodium-ios/lib/libsodium.a
	@mkdir -p target/ios
	cp $< $@

target/ios/libsecp256k1.a: deps/secp256k1/libsecp256k1-ios/lib/libsecp256k1.a
	@mkdir -p target/ios
	cp $< $@

target/js/libsecp256k1.a: deps/secp256k1/libsecp256k1-wasm/lib/libsecp256k1.a
	@mkdir -p target/js
	cp $< $@

target/js/libsodium.a: deps/libsodium/libsodium-js/lib/libsodium.a
	@mkdir -p target/js
	cp $< $@

deps/libsodium/libsodium-ios/lib/libsodium.a: deps/libsodium/configure
	cd deps/libsodium; \
	./dist-build/ios.sh

deps/secp256k1/libsecp256k1-ios/lib/libsecp256k1.a: deps/secp256k1/configure
	./tools/secp-ios.sh

deps/secp256k1/libsecp256k1-wasm/lib/libsecp256k1.a: deps/secp256k1/configure
	./tools/secp-wasm.sh

deps/libsodium/libsodium-js/lib/libsodium.a: deps/libsodium/configure
	cd deps/libsodium; \
	./dist-build/emscripten.sh --standard

deps/libsodium/src/libsodium/.libs/libsodium.a: deps/libsodium/config.log
	cd deps/libsodium/src/libsodium; \
	make -j libsodium.la

install: $(DEPS)
	mkdir -p $(PREFIX)/lib $(PREFIX)/include
	cp lnsocket.h $(PREFIX)/include
	cp lnsocket.a libsecp256k1.a libsodium.a $(PREFIX)/lib

install-js: js
	mkdir -p $(PREFIX)/share/lnsocket
	cp target/js/lnsocket.wasm target/js/lnsocket.js $(PREFIX)/share/lnsocket

install-all: install install-js

check: ctest
	./ctest
	npm test

gocheck:
	go test ./lnsocket.go

ctest: test.o $(DEPS)
	@echo "ld $@"
	@$(CC) $(CFLAGS) test.o $(OBJS) $(ARS) $(LDFLAGS) -o $@

lnrpc: lnrpc.o $(DEPS)
	@echo "ld $@"
	@$(CC) $(CFLAGS) lnrpc.o $(OBJS) $(ARS) $(LDFLAGS) -o $@

target/js/lnsocket.wasm: target/tmp/js/lnsocket.js
	cp target/tmp/js/lnsocket.wasm target/js/lnsocket.wasm

target/node/lnsocket.wasm: target/tmp/node/lnsocket.js
	cp target/tmp/node/lnsocket.wasm target/node/lnsocket.wasm

target/tmp/node/lnsocket.js: $(WASM_ARS) lnsocket_pre.js
	@mkdir -p target/tmp/node
	emcc --pre-js lnsocket_pre.js -s MODULARIZE -flto -s 'EXPORTED_FUNCTIONS=["_malloc", "_free"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' $(CFLAGS) -Wl,-whole-archive $(WASM_ARS) -Wl,-no-whole-archive -o $@

target/tmp/js/lnsocket.js: $(WASM_ARS) lnsocket_pre.js
	@mkdir -p target/tmp/js
	emcc --pre-js lnsocket_pre.js -s ENVIRONMENT=web -s MODULARIZE -flto -s 'EXPORTED_FUNCTIONS=["_malloc", "_free"]' -s 'EXPORTED_RUNTIME_METHODS=["ccall","cwrap"]' $(CFLAGS) -Wl,-whole-archive $(WASM_ARS) -Wl,-no-whole-archive -o $@

tags: fake
	find . -name '*.c' -or -name '*.h' | xargs ctags

clean: fake
	rm -rf $(BINS) config.h $(OBJS) $(ARM64_OBJS) $(X86_64_OBJS) $(WASM_OBJS) target

distclean: clean
	rm -rf $(ARS) liblnsocket.a deps target


.PHONY: fake
