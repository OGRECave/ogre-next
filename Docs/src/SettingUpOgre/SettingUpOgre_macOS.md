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

List all your available build options:
```bash
cmake -LA
```

At the time of writing this guide, a build error will occurr for the SampleMorphAnimations target (unless one is building .frameworks which this guide won't). So remove the following line line or comment it out, in `Samples/2.0/CMakeLists.txt:130` that says:
```
add_subdirectory(ApiUsage/MorphAnimations)
```

Then configure the options to compose your flavor of what you want to build by setting ogre and cmake variables. Examples of variables that this guide configures:
```
# Opt-out from OpenGL (Metal is already set to be built)
OGRE_BUILD_RENDERSYSTEM_GL3PLUS

# Build .dylib/.a files instead of .framework
OGRE_BUILD_LIBS_AS_FRAMEWORKS

# Make sure the RenderSystem.dylib is named libRenderSystem.dylib to conform to *NIX standard
OGRE_PLUGIN_LIB_PREFIX
```

Configure the above variables
```bash
cmake -D OGRE_BUILD_RENDERSYSTEM_GL3PLUS=OFF -D OGRE_BUILD_LIBS_AS_FRAMEWORKS=NO -D OGRE_PLUGIN_LIB_PREFIX=lib
```

Generate a Makefile
```
cmake .
```

Now, build ogre:
```
make install
```

If everything went well, you should have an sdk located in the `sdk/` directory.

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