#!/bin/bash

if [ -z "$OGRE_VERSION" ]; then
	echo "OGRE_VERSION is not set. Aborting!"
	exit 1;
fi

echo "--- Installing System Dependencies ---"
sudo apt-get update
sudo apt-get install -y ninja-build libxrandr-dev libxaw7-dev libxcb-randr0-dev libx11-xcb-dev libsdl2-dev doxygen graphviz

echo "--- Fetching prebuilt Dependencies ---"
wget https://github.com/OGRECave/ogre-next-deps/releases/download/bin-releases/Dependencies_Release_Ubuntu.20.04.LTS.Clang-12_a3f61e782f3effbd58a15727885cbd85cd1b342b.7z

echo "--- Extracting prebuilt Dependencies ---"
7z x Dependencies_Release_Ubuntu.20.04.LTS.Clang-12_a3f61e782f3effbd58a15727885cbd85cd1b342b.7z

mkdir -p build/Doxygen
cd build/Doxygen
echo "--- Building Ogre (Debug) ---"
cmake \
-DOGRE_CONFIG_THREAD_PROVIDER=0 \
-DOGRE_CONFIG_THREADS=0 \
-DOGRE_BUILD_COMPONENT_SCENE_FORMAT=1 \
-DOGRE_BUILD_SAMPLES2=0 \
-DOGRE_BUILD_TESTS=0 \
-DOGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS=1 \
-DCMAKE_BUILD_TYPE="Debug" \
-DCMAKE_CXX_STANDARD=11 \
-G Ninja ../.. || exit $?
ninja OgreDoc || exit $?

echo "--- Going to gh-pages repo ---"
cd ../../gh-pages || exit $?
cd api || exit $?
echo "--- Removing old ${OGRE_VERSION} ---"
git rm -rf ${OGRE_VERSION} || exit $?
rm -rf ${OGRE_VERSION} || exit $?
echo "--- Copying new ${OGRE_VERSION} ---"
mv ../../build/Doxygen/api/html ${OGRE_VERSION} || exit $?

git config user.email "darksylinc"
git config user.name "darksylinc@users.noreply.github.com"

echo "--- Adding to ${OGRE_VERSION} to git ---"
git add ${OGRE_VERSION} || exit $?
if [ $(git status --porcelain | wc -l) -eq "0" ]; then
	echo "--- Nothing has changed. Nothing to commit or push. ---"
else
	echo "--- Committing ---"
	git commit -m "Deploy GH" || exit $?
	echo "--- Pushing repo... ---"
	git push || exit $?
fi

echo "Done!"
