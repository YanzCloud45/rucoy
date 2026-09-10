#!/usr/bin/env bash
set -euo pipefail

: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to Android NDK r26d first}"

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

mkdir -p native/include
if [[ ! -s native/include/zygisk.hpp ]]; then
  curl -L --fail --retry 3 \
    https://raw.githubusercontent.com/topjohnwu/zygisk-module-sample/master/module/jni/zygisk.hpp \
    -o native/include/zygisk.hpp
fi

DOBBY_VERSION="1.2"
AAR_URL="https://repo.maven.apache.org/maven2/io/github/vvb2060/ndk/dobby/${DOBBY_VERSION}/dobby-${DOBBY_VERSION}.aar"

if [[ ! -s third_party/dobby-prebuilt/lib/libdobby.a || ! -s third_party/dobby-prebuilt/include/dobby.h ]]; then
  rm -rf third_party/dobby-prebuilt third_party/dobby-aar
  mkdir -p third_party/dobby-prebuilt/include third_party/dobby-prebuilt/lib third_party/dobby-aar

  curl -L --fail --retry 3 "$AAR_URL" -o third_party/dobby.aar
  unzip -q third_party/dobby.aar -d third_party/dobby-aar

  DOBBY_LIB="$(find third_party/dobby-aar -type f -name 'libdobby.a' | grep -E '/(android\.)?arm64-v8a/' | head -n 1 || true)"
  DOBBY_HEADER="$(find third_party/dobby-aar -type f -name 'dobby.h' | head -n 1 || true)"

  [[ -n "$DOBBY_LIB" ]] || { echo "arm64-v8a libdobby.a not found"; exit 1; }
  [[ -n "$DOBBY_HEADER" ]] || { echo "dobby.h not found"; exit 1; }

  cp "$DOBBY_LIB" third_party/dobby-prebuilt/lib/libdobby.a
  cp "$DOBBY_HEADER" third_party/dobby-prebuilt/include/dobby.h
fi

rm -rf build
cmake -S native -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel

rm -rf out
mkdir -p out/module/zygisk
cp -a module/. out/module/
rm -f out/module/zygisk/.gitkeep
cp build/librucoy_tile_bot.so out/module/zygisk/arm64-v8a.so
chmod 0755 out/module/customize.sh out/module/action.sh

(
  cd out/module
  zip -9 -r ../Rucoy-TileBot-Zygisk.zip .
)

echo "Built: out/Rucoy-TileBot-Zygisk.zip"
