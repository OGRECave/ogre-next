#!/bin/bash

OGRE_BRANCH_NAME="{0}"

mkdir Ogre
cd Ogre
if test ! -f ogredeps; then
	mkdir ogredeps
	echo "--- Cloning Ogredeps ---"
	hg clone https://bitbucket.org/cabalistic/ogredeps ogredeps
else
	echo "--- Ogredeps repo detected. Cloning skipped ---"
fi
cd ogredeps
mkdir build
cd build
echo "--- Building Ogredeps ---"
cmake {1} -G Ninja ..
ninja
ninja install

cd ../../
if test ! -f ogre-next; then
	mkdir ogre-next
	echo "--- Cloning Ogre {0} ---"
	git clone --branch ${{OGRE_BRANCH_NAME}} https://github.com/OGRECave/ogre-next
fi
cd ogre-next
if test ! -f Dependencies; then
	ln -s ../ogredeps/build/ogredeps Dependencies
fi
mkdir -p build/Debug
mkdir -p build/Release
cd build/Debug
echo "--- Building Ogre (Debug) ---"
cmake -D OGRE_USE_BOOST=0 -D OGRE_CONFIG_THREAD_PROVIDER=0 -D OGRE_CONFIG_THREADS=0 -D OGRE_BUILD_COMPONENT_SCENE_FORMAT=1 -D OGRE_BUILD_SAMPLES2=1 -D OGRE_BUILD_TESTS=1 -D CMAKE_BUILD_TYPE="Debug" {1} -G Ninja ../..
ninja
cd ../Release
echo "--- Building Ogre (Release) ---"
cmake -D OGRE_USE_BOOST=0 -D OGRE_CONFIG_THREAD_PROVIDER=0 -D OGRE_CONFIG_THREADS=0 -D OGRE_BUILD_COMPONENT_SCENE_FORMAT=1 -D OGRE_BUILD_SAMPLES2=1 -D OGRE_BUILD_TESTS=1 -D CMAKE_BUILD_TYPE="Release" {1} -G Ninja ../..
ninja

echo "Done!"
