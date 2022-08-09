#! /usr/bin/env bash
#
#  Step 1.
#  Configure for base system so simulator is covered
#  
#  Step 2.
#  Make for iOS and iOS simulator
#
#  Step 3.
#  Merge libs into final version for xcode import

cd deps/secp256k1

export PREFIX="$(pwd)/libsecp256k1-ios"
export IOS64_PREFIX="$PREFIX/tmp/ios64"
export SIMULATOR64_PREFIX="$PREFIX/tmp/simulator64"
export XCODEDIR=$(xcode-select -p)
export IOS_SIMULATOR_VERSION_MIN=${IOS_SIMULATOR_VERSION_MIN-"6.0.0"}
export IOS_VERSION_MIN=${IOS_VERSION_MIN-"6.0.0"}

mkdir -p $SIMULATOR64_PREFIX $IOS64_PREFIX || exit 1

# Build for the simulator
export BASEDIR="${XCODEDIR}/Platforms/iPhoneSimulator.platform/Developer"
export PATH="${BASEDIR}/usr/bin:$BASEDIR/usr/sbin:$PATH"
export SDK="${BASEDIR}/SDKs/iPhoneSimulator.sdk"

## x86_64 simulator
export CFLAGS="-O2 -arch x86_64 -isysroot ${SDK} -mios-simulator-version-min=${IOS_SIMULATOR_VERSION_MIN} -flto"
export LDFLAGS="-arch x86_64 -isysroot ${SDK} -mios-simulator-version-min=${IOS_SIMULATOR_VERSION_MIN} -flto"

make distclean > /dev/null

./configure --host=x86_64-apple-darwin10 --disable-shared --enable-module-ecdh --enable-module-schnorrsig --enable-module-extrakeys --prefix="$SIMULATOR64_PREFIX" 

make -j3 install || exit 1

# Build for iOS
export BASEDIR="${XCODEDIR}/Platforms/iPhoneOS.platform/Developer"
export PATH="${BASEDIR}/usr/bin:$BASEDIR/usr/sbin:$PATH"
export SDK="${BASEDIR}/SDKs/iPhoneOS.sdk"

## 64-bit iOS
export CFLAGS="-O2 -arch arm64 -isysroot ${SDK} -mios-version-min=${IOS_VERSION_MIN} -flto -fembed-bitcode"
export LDFLAGS="-arch arm64 -isysroot ${SDK} -mios-version-min=${IOS_VERSION_MIN} -flto -fembed-bitcode"

make distclean > /dev/null

./configure --host=arm-apple-darwin10 --disable-shared --enable-module-ecdh --enable-module-schnorrsig --enable-module-extrakeys --prefix="$IOS64_PREFIX" || exit 1

make -j3 install || exit 1

# Create universal binary and include folder
rm -fr -- "$PREFIX/include" "$PREFIX/libsecp256k1.a" 2> /dev/null
mkdir -p -- "$PREFIX/lib"
lipo -create \
  "$SIMULATOR64_PREFIX/lib/libsecp256k1.a" \
  "$IOS64_PREFIX/lib/libsecp256k1.a" \
  -output "$PREFIX/lib/libsecp256k1.a"

echo
echo "libsecp256k1 has been installed into $PREFIX"
echo
file -- "$PREFIX/lib/libsecp256k1.a"

# Cleanup
rm -rf -- "$PREFIX/tmp"
make distclean > /dev/null
