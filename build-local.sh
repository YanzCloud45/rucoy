#!/usr/bin/env bash
set -euo pipefail

: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME first}"

if [[ ! -f native/include/zygisk.hpp ]]; then
  echo "native/include/zygisk.hpp missing. Download the published Zygisk header first."
  exit 1
fi

if [[ ! -d third_party/Dobby/.git ]]; then
  git clone https://github.com/jmpews/Dobby.git third_party/Dobby
  git -C third_party/Dobby checkout 809f8ca
fi

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
(
  cd out/module
  zip -9 -r ../Rucoy-TileBot-Zygisk.zip .
)

echo "Built: out/Rucoy-TileBot-Zygisk.zip"
