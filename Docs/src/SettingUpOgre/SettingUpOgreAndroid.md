# Android {#SettingUpOgreAndroid}

@tableofcontents

# Requirements {#RequirementsAndroid}
 - [CMake 3.x](https://cmake.org/download/)
 - Git
 - Android Studio 4.0
 - Android SDK & NDK
 - Python 3.x (to build the samples)
 - Vulkan-capable Android phone.
 - Android 8.0 or newer strongly recommended. Android 7.0 and 7.1 are supported,
 but most phones are bundled with very old and buggy drivers.
 - For HW & SW requirements, please visit http://www.ogre3d.org/developers/requirements

# Downloading and building Ogre {#DownloadingOgreAndroid}

This guide has been written with Linux as the host machine in mind, however it should
be possible to port it to Windows with ease.

All that's needed is git, the ability to create folders, CMake, and Android SDK & NDK.

We're going to assume:
  - Android SDK is stored at /home/username/Android/Sdk/
  - Android NDK is stored at /home/username/Android/Sdk/ndk/21.0.6113669
  - Your working dir is /home/username/workingdir
  - Target ABI is arm64-v8a

You can of course change these by modifying the commands in this guide.

```sh

cd /home/username/workingdir

# Download ogre-next-deps
git clone --recurse-submodules --shallow-submodules https://github.com/OGRECave/ogre-next-deps

# Compile ogre-next-deps
cd ogre-next-deps
mkdir -p build/Android/Release
cd build/Android/Release
cmake \
    -DCMAKE_TOOLCHAIN_FILE=/home/username/Android/Sdk/ndk/21.0.6113669/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_NATIVE_API_LEVEL=24 \
    -DCMAKE_BUILD_TYPE=Release \
    ../../../
make -j9
make install

cd ../../../

# Download ogre-next
git clone --branch master https://github.com/OGRECave/ogre-next

cd ogre-next
ln -s ../ogre-next-deps/build/Android/Release/ogredeps DependenciesAndroid

# Compile ogre-next (Debug)
mkdir -p build/Android/Debug
cd build/Android/Debug
cmake \
    -DCMAKE_TOOLCHAIN_FILE=/home/username/Android/Sdk/ndk/21.0.6113669/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_NATIVE_API_LEVEL=24 \
    -DOGRE_BUILD_PLATFORM_ANDROID=1 \
    -DOGRE_DEPENDENCIES_DIR=/home/username/workingdir/ogre-next/DependenciesAndroid \
    -DOGRE_SIMD_NEON=OFF \
    -DOGRE_SIMD_SSE2=OFF \
    -DCMAKE_BUILD_TYPE=Debug \
    ../../../
make -j9
make install

cd ../../../

# Compile ogre-next (Release)
mkdir -p build/Android/Release
cd build/Android/Release
cmake \
    -DCMAKE_TOOLCHAIN_FILE=/home/username/Android/Sdk/ndk/21.0.6113669/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_NATIVE_API_LEVEL=24 \
    -DOGRE_BUILD_PLATFORM_ANDROID=1 \
    -DOGRE_DEPENDENCIES_DIR=/home/username/workingdir/ogre-next/DependenciesAndroid \
    -DOGRE_SIMD_NEON=OFF \
    -DOGRE_SIMD_SSE2=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    ../../../
make -j9
make install
```

# Building Samples {#BuildingSamplesAndroid}

The samples are built alongside Ogre (unless OGRE_BUILD_SAMPLES2 is not set)
in the previous sectio in the form of C++ static libraries.

However Android Studio is needed for the final step to compile a bit of C++, link all libraries
(i.e. libOgreSamplesCommon.a, libSampleName.a and all Ogre libraries) and generate
the APK file.

The Android Studio projects for each sample are generated from a single template located at
/home/username/workingdir/ogre-next/Samples/2.0/AndroidAppTemplate

The script GenTemplate.py will duplicate and customize the template for all samples.
i.e. simply run:

```sh
cd /home/username/workingdir/ogre-next/Samples/2.0/AndroidAppTemplate
python3 GenTemplate.py
```

This will generate a lot of Android Studio projects, one per sample, at /home/username/workingdir/ogre-next/Samples/2.0/AndroidAppTemplate/Autogen

Open these projects in Android Studio and deploy the APK.

> ⚠️ Notes:
>
> - Any change to the code in AndroidAppTemplate can be broadcasted to
> all samples by running GenTemplate.py again
> - If you get weird debugging issues e.g. messed up callstacks, breakpoints set in Android Studio aren't hit when they should. In Android Studio select `Build->Rebuild Project`
> - Android Studio likes to copy lib files to temp folders, creating duplicates. Thus compiling ALL samples can take A LOT of disk space (in the several hundred GBs if you build them all).
> - AndroidAppTemplate by default includes validation layers in Debug builds. The validation layers will cause a failure to run the sample on Android <= 7.1, thus remove them from build.gradle if you're affected
> - To get the validation layers running on Android 7.x, you must compile them yourself [from repo](https://github.com/KhronosGroup/Vulkan-ValidationLayers/tree/master/build-android) but first you must comment any reference to AHardwareBuffer_describe

> ⚠️ Note:
>
> Building for Android force-enables OGRE_CONFIG_ENABLE_VIEWPORT_ORIENTATIONMODE
>
> This setting [improves performance and/or lowers battery consumption](https://community.arm.com/developer/tools-software/graphics/b/blog/posts/appropriate-use-of-surface-rotation) on phones [without HW-accelerated preTransform rotation](https://arm-software.github.io/vulkan_best_practice_for_mobile_developers/samples/performance/surface_rotation/surface_rotation_tutorial.html).
>
> However OGRE_CONFIG_ENABLE_VIEWPORT_ORIENTATIONMODE can introduce bugs either in Ogre or your application
> e.g. see Tutorial_SSAOGameState::update which must call `camera->setOrientationMode( Ogre::OR_DEGREE_0 )`
> to properly apply SSAO.
>
> For example:
>  - { dFdx, dFdx } [needs to be mapped](https://android-developers.googleblog.com/2020/02/handling-device-orientation-efficiently.html) to {dFdy, -dFdx}, {-dFdy, dFdx}, {-dFdx, -dFdy}, for 90°, 270° and 180° respectively
>  - The same happens to gl_FragCoord
>
> This is aggravated by the fact that the bug may not manifest itself if the phone is in portrait mode
> (but ocassionally does manifest even in portrait).
>
> If you suspect this is the cause of your bugs, edit the CMakeLists.txt file so that
> OGRE_CONFIG_ENABLE_VIEWPORT_ORIENTATIONMODE is no longer force-enabled.
>
> You can debug viewport orientation bugs on Desktop by enabling OGRE_CONFIG_ENABLE_VIEWPORT_ORIENTATIONMODE
> in your Desktop build, and manually editing OgreVulkanWindow.cpp and set:
>
> `surfaceCaps.currentTransform = VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;`
>
> before Ogre uses its value.
>
> The window will be 90° rotated, but you will be able
> to reproduce the bug on a desktop, where it is much easier to solve than on a phone.