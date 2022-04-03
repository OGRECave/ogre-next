# macOS {#SettingUpOgre_macOS}

@tableofcontents

# Install cmake
If you do not already have `cmake`, install it with `brew` or whatever your favorite way is:
```bash
brew install cmake
```

@copydoc DownloadingOgreScriptsCommon

# Install dependencies

Clone dependencies from [ogre-next-deps](https://github.com/OGRECave/ogre-next-deps) and build the `ALL` target and then the `INSTALL` target:

```bash
git clone --recurse-submodules --shallow-submodules https://github.com/OGRECave/ogre-next-deps
cd ogre-next-deps
mkdir build
cd build
cmake ../ -G Xcode
cmake --build . --target ALL --config Debug
cmake --build . --target install --config Debug
cmake --build . --target ALL --config Release
cmake --build . --target install --config Release
```

This will generate Dependencies under `ogre-next-deps/build/ogrenext`.
You will have to copy (or symlink) that folder into the OgreRepo and rename it to dependencies so that they the folder `ogre-next-deps/build/ogrenext` is now `OgreSrcRepo/Dependencies`


## Install SDL2 using brew

SDL2 is not built by ogre-next-deps in macOS, use brew instead:

```bash
brew install sdl2
```


# Build Ogre

> **Note:** XCode generator is supported
>
> **Note 2:** Unless STATIC libs setting is used, XCode project dependencies are not properly setup thus e.g. if you try to build a sample, it won't compile RenderSystem_Metal which is needed (and possibly other components). Make sure to first build "ALL" target to build everything before trying the samples

Make sure you have set the current working directory to ogre's source directory:
```bash
cd ogre
```

At the time of writing this guide, a build error will occurr for the SampleMorphAnimations target (unless one is building .frameworks which this guide won't). So remove the following line line or comment it out, in `Samples/2.0/CMakeLists.txt:130` that says:
> add_subdirectory(ApiUsage/MorphAnimations)

Then configure the options to compose your flavor of what you want to build by setting ogre and cmake variables. Examples of variables that this guide configures:
|  Varible   |  What this guide will use it for   |
|---  |---  |
|OGRE_BUILD_RENDERSYSTEM_GL3PLUS     |  Opt-out from OpenGL (Metal is already set to be built by default)   |
|  OGRE_BUILD_LIBS_AS_FRAMEWORKS   |  Build .dylib/.a files instead of .framework   |
|  OGRE_PLUGIN_LIB_PREFIX   |  Make sure the RenderSystem.dylib is named libRenderSystem.dylib to conform to *NIX standard   |
|  CMAKE_INSTALL_PREFIX   |  Directory in which CMake should copy the final SDK files. A `lib/` `/bin` and so on will be automatically created inside here   |
|  CMAKE_INSTALL_NAME_DIR   |  Directory of where the installed target file is located, i.e. should be `$CMAKE_INSTALL_PREFIX/lib`. This will be imprinted in the target file itself, and can be verfied after install by e.g. `otool -L $CMAKE_INSTALL_PREFIX/lib/OgreMain.dylib`   |
|  CMAKE_CXX_STANDARD   |  Compile with c++11 support   |
|  CMAKE_CXX_FLAGS   |  Set explicit include directory. (C)Make [seems to ignore](https://gitlab.kitware.com/cmake/cmake/-/issues/19180) this system directory otherwise.   |


You can list all your available build options and adjust for your needs:
```bash
cmake -LA -N
```

Configure the above variables (`$PWD` will be replaced with the current working directory)
```bash
cmake  -D OGRE_BUILD_RENDERSYSTEM_GL3PLUS=OFF \
       -D OGRE_BUILD_LIBS_AS_FRAMEWORKS=NO \
       -D OGRE_PLUGIN_LIB_PREFIX=lib \
       -D CMAKE_CXX_STANDARD=11 \
       -D CMAKE_INSTALL_PREFIX=$PWD/sdk \
       -D CMAKE_CXX_FLAGS="-I/usr/local/include -F/Library/Frameworks" \
       -D CMAKE_INSTALL_NAME_DIR=$PWD/sdk/lib .

```

Generate a Makefile
```
cmake .
```

Now, build ogre:
```
make -j2 install
```

The `-j2` tells make how many jobs will run concurrently. E.g. if you have 4 cores, use 4 instead of 2.

If everything went well, you should have an sdk located in the `$PWD/sdk` directory.

Open and edit `$PWD/sdk/plugins_tools_d.cfg` and `$PWD/sdk/plugins_tools.cfg` and the value of `PluginFolder` to $PWD/sdk/lib` and prefix all the `Plugin` values with `lib` (because we set the `$OGRE_PLUGIN_LIB_PREFIX` when building)

# Build offline version of the docs
From the same working directory run:
```
brew install doxygen
brew install Graphviz
```

and then run the following in the directory where the `html.cfg` file is located:
```
doxygen html.cfg
```

Doc files should now be located at `sdk/Docs/api/index.html` or whatever the `OUTPUT_DIRECTORY` is set to in `/html.cfg`.