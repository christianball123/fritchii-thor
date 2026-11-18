#!/usr/bin/env bash
# Configure a Fritchii tree for AYN Thor (Snapdragon 8 Gen 2 / aarch64).
# Run from the Fritchii source root after applying patches/duo.
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build/release}"
BUILD_TAG="${BUILD_TAG:-thor-local}"
BUILD_PLATFORM="${BUILD_PLATFORM:-linux-aarch64}"
PGO="${PGO:-0}"
PGO_LLVM_VERSION="${PGO_LLVM_VERSION:-23}"

tune="-march=armv8.2-a+crypto+fp16+rcpc+dotprod -mtune=cortex-x3"
# Ubuntu 24.04 arm64 GCC + Fritchii's -Werror=all promotes stock warnings that
# x86_64 release builders don't hit the same way. -Wno-error=<cat> must be used
# (plain -Wno-error does not cancel -Werror=all). Keep warnings visible.
werror_relax="-Wno-error=stringop-truncation -Wno-error=stringop-overflow \
-Wno-error=stringop-overread -Wno-error=array-bounds -Wno-error=maybe-uninitialized \
-Wno-error=restrict -Wno-error=format-truncation -Wno-error=format-overflow \
-Wno-error=use-after-free -Wno-error=free-nonheap-object -Wno-error=uninitialized"
cflags="$tune $werror_relax"
cxxflags="$tune $werror_relax"

flags=(
  -DCMAKE_BUILD_TYPE=Release
  -DBUILD_TESTING=OFF
  -DYUZU_TESTS=OFF
  -DYUZU_CMD=ON
  -DYUZU_ROOM=OFF
  -DYUZU_ROOM_STANDALONE=OFF
  -DYUZU_USE_BUNDLED_QT=OFF
  -DYUZU_USE_BUNDLED_SDL3=ON
  -DENABLE_UPDATE_CHECKER=OFF
  -DUSE_DISCORD_PRESENCE=OFF
  -DUSE_CCACHE=ON
  -DENABLE_WERROR=OFF
  "-DCMAKE_C_FLAGS=$cflags"
  "-DCMAKE_CXX_FLAGS=$cxxflags"
)

# Prefer Fritchii / Eden style build tag vars when present.
if grep -q 'FRITCHII_BUILD_TAG\|YUZU_BUILD_TAG\|EDEN_BUILD_TAG' CMakeLists.txt 2>/dev/null \
  || grep -rq 'FRITCHII_BUILD_TAG\|YUZU_BUILD_TAG' CMakeModules 2>/dev/null; then
  :
fi
# Pass common tag vars; unknown -D keys are ignored by CMake unless marked required.
flags+=("-DYUZU_BUILD_TAG=$BUILD_TAG" "-DFRITCHII_BUILD_PLATFORM=$BUILD_PLATFORM")

if [ "$PGO" = "1" ]; then
  test -s eglantine.profdata
  profile="-fprofile-use=$PWD/eglantine.profdata -Wno-backend-plugin \
    -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date $tune"
  flags+=(
    "-DCMAKE_C_COMPILER=clang-$PGO_LLVM_VERSION"
    "-DCMAKE_CXX_COMPILER=clang++-$PGO_LLVM_VERSION"
    "-DCMAKE_EXE_LINKER_FLAGS=-B/usr/lib/llvm-$PGO_LLVM_VERSION/bin -fuse-ld=lld"
    "-DCMAKE_C_FLAGS=$profile"
    "-DCMAKE_CXX_FLAGS=$profile"
  )
fi

cmake -S . -B "$BUILD_DIR" -G Ninja "${flags[@]}"
echo "Configured $BUILD_DIR. Build with: cmake --build $BUILD_DIR --target eden eden-cli"
