# Linux {#SettingUpOgreLinux}

@tableofcontents

# Requirements {#RequirementsLinux}
    * [CMake 3.x](https://cmake.org/download/)
    * Git
    * What you do **NOT** need: Boost. Don't waste your time.
    * Clang >3.5 or GCC >4.0
    * [QtCreator](https://download.qt.io/official_releases/qtcreator/) recommended (Optional).
    * Debian-based: `sudo apt-get install libfreetype6-dev libfreeimage-dev libzzip-dev libxrandr-dev libxaw7-dev freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev doxygen graphviz python-clang-4.0`
    * Arch: `pacman -S freeimage freetype2 libxaw libxrandr mesa zziplib cmake gcc`
    * For HW & SW requirements, please visit http://www.ogre3d.org/developers/requirements
    * NVIDIA users: Proprietary drivers are recommended.

@copydoc DownloadingOgreScriptsCommon

# Downloading Ogre {#DownloadingOgreLinux}

@copydoc DownloadingOgreCommon

# Building Dependencies {#BuildingDependenciesLinux}

```sh
cd Ogre/Dependencies
mkdir build
cd build
cmake ../
# Optionally configure build to use distro packages freeimage, freetype, and zlib if installed
# cmake -D OGREDEPS_BUILD_FREEIMAGE=0 -D OGREDEPS_BUILD_FREETYPE=0 -D OGREDEPS_BUILD_ZLIB=0 ../
cd build
make
make install```


# Building Ogre {#BuildingOgreLinux}

We'll create both a Release & Debug configuration that match the ones used in Windows.
This eases portability and cross platform development.
```sh
cd Ogre
mkdir build
cd build
mkdir Debug
mkdir Release
# Build Debug
cd Debug
cmake -D OGRE_DEPENDENCIES_DIR=Dependencies/build/ogredeps -D OGRE_BUILD_SAMPLES2=1 -D OGRE_USE_BOOST=0 -D OGRE_CONFIG_THREAD_PROVIDER=0 -D OGRE_CONFIG_THREADS=0 -D CMAKE_BUILD_TYPE=Debug ../../
make
make install
# Build Release
cd ../Release
cmake -D OGRE_DEPENDENCIES_DIR=Dependencies/build/ogredeps -D OGRE_BUILD_SAMPLES2=1 -D OGRE_USE_BOOST=0 -D OGRE_CONFIG_THREAD_PROVIDER=0 -D OGRE_CONFIG_THREADS=0 -D CMAKE_BUILD_TYPE=Release ../../
make
make install
```


# Setting Up Ogre with QtCreator {#SettingUpOgreWithQtCreatorLinux}
TBD
