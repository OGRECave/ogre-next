# macOS {#SettingUpOgre_macOS}

@tableofcontents

# Install cmake
If you do not already have `cmake`, install it with `brew` or whatever your favorite way is:
```bash
brew install cmake
```

# Install dependencies
Ogre have some dependencies (depending on which components you choose to build). You can find them at [https://bitbucket.org/cabalistic/ogredeps/src/default/src/](https://bitbucket.org/cabalistic/ogredeps/src/default/src/). This guide won't escribe in detail on how to install them, but the author recommends using `brew` to install them, e.g.
```bash
brew install zzlib
brew install freetype
brew install rapidjson
# [...more if needed...]
```
and cmake should be able to find them automatically.

# Build Ogre
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


You can list all your available build options and adjust for your needs:
```bash
cmake -LA -N
```

Configure the above variables
```bash
cmake  -D OGRE_BUILD_RENDERSYSTEM_GL3PLUS=OFF \
       -D OGRE_BUILD_LIBS_AS_FRAMEWORKS=NO \
       -D OGRE_PLUGIN_LIB_PREFIX=lib \
       -D CMAKE_INSTALL_PREFIX=/put/final/sdk/here \
       -D CMAKE_INSTALL_NAME_DIR=/put/final/sdk/here/lib

```

Generate a Makefile
```
cmake .
```

Now, build ogre:
```
make install
```

If everything went well, you should have an sdk located in the `/put/final/sdk/here` directory.

# Build offline version of the docs
From the same working directory run:
```
brew install doxygen
brew install Graphviz
```

and then
```
doxygen html.cfg
```

Doc files should now be located at `sdk/Docs/api/index.html` or whatever the `OUTPUT_DIRECTORY` is set to in `/html.cfg`.