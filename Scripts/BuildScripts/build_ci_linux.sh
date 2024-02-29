#!/bin/bash

echo "--- Setting Clang as default compiler ---"
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

echo "--- Installing System Dependencies ---"
sudo apt-get update
sudo apt-get install -y ninja-build libxrandr-dev libxaw7-dev libxcb-randr0-dev libx11-xcb-dev libsdl2-dev

echo "--- Fetching prebuilt Dependencies ---"
wget https://github.com/OGRECave/ogre-next-deps/releases/download/bin-releases/Dependencies_Release_Ubuntu.20.04.LTS.Clang-12_22ab475b517e724409cb6d585b3646ac6a7b23ea.7z

echo "--- Extracting prebuilt Dependencies ---"
7z x Dependencies_Release_Ubuntu.20.04.LTS.Clang-12_22ab475b517e724409cb6d585b3646ac6a7b23ea.7z

mkdir -p build/Debug
cd build/Debug
echo "--- Building Ogre (Debug) ---"
cmake \
-DOGRE_CONFIG_THREAD_PROVIDER=0 \
-DOGRE_CONFIG_THREADS=0 \
-DOGRE_BUILD_COMPONENT_SCENE_FORMAT=1 \
-DOGRE_BUILD_SAMPLES2=1 \
-DOGRE_BUILD_TESTS=1 \
-DOGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS=1 \
-DCMAKE_BUILD_TYPE="Debug" \
-DCMAKE_CXX_STANDARD=11 \
-DCMAKE_INSTALL_PREFIX="./build/SDK_install/" \
-G Ninja ../.. || exit $?
ninja || exit $?
ninja install || exit $?

echo "Done!"
