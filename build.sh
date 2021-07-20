#!/bin/bash
set -e

cd "$(dirname ${BASH_SOURCE[0]})"

mkdir -p build
pushd build >/dev/null

cmake .. -G "Visual Studio 16 2019" \
         -DCMAKE_BUILD_TYPE=Release

msbuild geometrypatch.vcxproj -p:configuration=release
