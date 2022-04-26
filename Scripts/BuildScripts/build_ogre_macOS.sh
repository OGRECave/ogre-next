#!/bin/bash

OGRE_BRANCH_NAME="{0}"

mkdir Ogre
cd Ogre
if test ! -d ogre-next-deps; then
	mkdir ogre-next-deps
	echo "--- Cloning ogre-next-deps ---"
	git clone --recurse-submodules --shallow-submodules https://github.com/OGRECave/ogre-next-deps || exit $?
else
	echo "--- ogre-next-deps repo detected. Cloning skipped ---"
fi
cd ogre-next-deps
mkdir build
cd build
echo "--- Building ogre-next-deps ---"
cmake -G Xcode .. || exit $?
cmake --build . --target install --config Debug || exit $?
cmake --build . --target install --config RelWithDebInfo || exit $?

cd ../../
if test ! -d ogre-next; then
	mkdir ogre-next
	echo "--- Cloning Ogre-Next {0} ---"
	git clone --branch ${{OGRE_BRANCH_NAME}} https://github.com/OGRECave/ogre-next || exit $?
fi
cd ogre-next
if test ! -L Dependencies; then
	ln -s ../ogre-next-deps/build/ogredeps Dependencies
fi

mkdir -p build
cd build
echo "--- Configuring Ogre-Next ---"
cmake -D OGRE_CONFIG_THREAD_PROVIDER=0 \
-D OGRE_CONFIG_THREADS=0 \
-D OGRE_BUILD_COMPONENT_SCENE_FORMAT=0 \
-D OGRE_BUILD_SAMPLES2=1 \
-D OGRE_BUILD_TESTS=0 \
-D OGRE_BUILD_RENDERSYSTEM_GL3PLUS=0 \
-D OGRE_BUILD_LIBS_AS_FRAMEWORKS=0 \
-G Xcode ../ || exit $?
echo "--- Building Ogre (Debug) ---"
cmake --build . --target ALL_BUILD --config Debug || exit $?
echo "--- Building Ogre (Release) ---"
cmake --build . --target ALL_BUILD --config RelWithDebInfo || exit $?

echo "Done!"
