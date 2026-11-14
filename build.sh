#!/bin/bash

set -e

cd "$(dirname "$0")"

rm -rf build
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
cmake --build . -j

echo "Build completed. Run: ./build/watermelon.exe"