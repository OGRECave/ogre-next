#!/bin/bash

OGRE_BRANCH_NAME="master"

mkdir Ogre
cd Ogre
if test ! -f ogre-next-deps; then
	mkdir ogre-next-deps
	echo "--- Cloning ogre-next-deps ---"
	git clone --recurse-submodules --shallow-submodules https://github.com/OGRECave/ogre-next-deps
else
	echo "--- ogre-next-deps repo detected. Cloning skipped ---"
fi
cd ogre-next-deps
mkdir build
cd build
echo "--- Building ogre-next-deps ---"
cmake  -G Ninja ..
ninja
ninja install

cd ../../
if test ! -f ogre-next; then
	mkdir ogre-next
	echo "--- Cloning Ogre master ---"
	git clone --branch ${OGRE_BRANCH_NAME} https://github.com/OGRECave/ogre-next
fi
cd ogre-next
if test ! -f Dependencies; then
	ln -s ../ogre-next-deps/build/ogredeps Dependencies
fi
mkdir -p build/Debug
mkdir -p build/Release
cd build/Debug
echo "--- Building Ogre (Debug) ---"
cmake -D OGRE_USE_BOOST=0 -D OGRE_CONFIG_THREAD_PROVIDER=0 -D OGRE_CONFIG_THREADS=0 -D OGRE_BUILD_COMPONENT_SCENE_FORMAT=1 -D OGRE_BUILD_SAMPLES2=1 -D OGRE_BUILD_TESTS=1 -D CMAKE_BUILD_TYPE="Debug"  -G Ninja ../..
ninja
cd ../Release
echo "--- Building Ogre (Release) ---"
cmake -D OGRE_USE_BOOST=0 -D OGRE_CONFIG_THREAD_PROVIDER=0 -D OGRE_CONFIG_THREADS=0 -D OGRE_BUILD_COMPONENT_SCENE_FORMAT=1 -D OGRE_BUILD_SAMPLES2=1 -D OGRE_BUILD_TESTS=1 -D CMAKE_BUILD_TYPE="Release"  -G Ninja ../..
ninja

echo "Done!"
