# What's new in Ogre-Next 3.0 {#Ogre30Changes}

@tableofcontents

## Ogre to OgreNext name migration

We're trying to make [OgreNext and Ogre able to be installed side-by-side](https://github.com/OGRECave/ogre-next/issues/232).

This is particularly important in Linux systems which rely on shared, centralized packages managed by package managers.

However this also benefits affect Windows apps that are able to select between Ogre and OgreNext at runtime.

One of the main issues is that DLLs are the same. i.e. `OgreMain.dll` or `libOgreMain.so` may refer to either Ogre or OgreNext.

That's why **we added the CMake option `OGRE_USE_NEW_PROJECT_NAME`**

When enabled, the project names will be `OgreNext` instead of `Ogre`. e.g. the following projects are affected (enunciative):

| Old Name                   | New Name                       |
|----------------------------|--------------------------------|
| OgreMain.dll               | OgreNextMain.dll               |
| OgreHlmsPbs.dll            | OgreNextHlmsPbs.dll            |
| OgreHlmsUnlit.dll          | OgreNextHlmsUnlit.dll          |
| OgreMeshLodGenerator.dll   | OgreNextMeshLodGenerator.dll   |
| OgreOverlay.dll            | OgreNextOverlay.dll            |
| OgreSceneFormat.dll        | OgreNextSceneFormat.dll        |
| libOgreMain.so             | libOgreNextMain.so             |
| libOgreHlmsPbs.so          | libOgreNextHlmsPbs.so          |
| libOgreHlmsUnlit.so        | libOgreNextHlmsUnlit.so        |
| libOgreMeshLodGenerator.so | libOgreNextMeshLodGenerator.so |
| libOgreOverlay.so          | libOgreNextOverlay.so          |
| libOgreSceneFormat.so      | libOgreNextSceneFormat.so      |

We understand this change can wreak havoc on our users who have scripts expecting to find OgreMain instead of OgreNextMain.

**Which is why this option will be off by default in OgreNext 3.0;
but will be turned on by default in OgreNext 4.0**

**The CMake option is scheduled for removal in OgreNext 5.0**

`EmptyProject`'s and Android's scripts have been updated to autodetect which name is being used and select between Ogre and OgreNext and CMake config time.

Make sure to upgrade to latest CMake scripts if you're using them; to be ready for all changes.

## PBS Changes in 3.0

Default material BRDF settings have changed in 3.0; thus materials will look different.

See [PBR / PBS Changes in 3.0](@ref PBSChangesIn30) to how make them look like they did in 2.3 and what these changes mean.

## Hlms Shader piece changes

The piece block `LoadNormalData` got split into `LoadGeomNormalData` & `LoadNormalData` in order to support Decals in Terra.

If you were overriding `LoadNormalData` in a custom piece, make sure to account for the new `LoadGeomNormalData`.

## AbiCookie

Creating Root changed slightly. One must now provide the Ogre::AbiCookie:

```cpp
const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();
mRoot = OGRE_NEW Ogre::Root( &abiCookie, pluginsPath, cfgPath,
                         mWriteAccessFolder + "Ogre.log", windowTitle );
```

The ABI cookie is a verification step to validate the version of OgreNext your project was compiled against is the same one as the library being loaded. This includes relevant CMake options that would cause the ABI to change and cause subtle runtime corruption (e.Debug vs Release, `OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS`, `OGRE_FLEXIBILITY_LEVEL`, `OGRE_CONFIG_THREAD_PROVIDER`, etc).

See Ogre::testAbiCookie for information of what to look for if the ABI cookie fails.

The AbiCookie may not catch all possible ABI mismatch issues, but it will catch the most common known ones.

Note that Ogre::generateAbiCookie is `FORCEINLINE` because it must see your project's settings.

## Move to C++11 and general cleanup

Lots of dead \& long-deprecated code was removed.
See [Resolving Merge Conflicts in Ogre-Next 3.0](@ref ResolvingMergeConflicts30) for more help with C++11 changes.

 - Remove D3D9
 - Remove Terrain
 - Remove RTShaderSystem
 - Remove NaCL
 - Remove dead SceneManagers
 - Remove `( void )` from empty functions
 - Remove StaticGeometry
 - Remove files under `Deprecated/` folders
 - Move to C++11
   - Remove code under `__cplusplus` that uses `< 201103L` `>= 201103L` or numbers below 201103L
 - Math::Log2 should use log2
 - All virtuals must have `overload` keyword
 - Remove `HardwareUniformBuffer`, `HardwareCounterBuffer`
 - Fix many warnings.
 - Clean up Media folder and remove unused stuff from Ogre samples.
 - Add ASAN CMake option.
 - Use OGRE_DEPRECATED.
 - Add cmake option to embed debug level into OgreBuildSettings.
    - Bakes OGRE_DEBUG_MODE into OgreBuildSettings.h; which is on by default on Ninja/GNU Make generators and disabled for the rest.
 - Pass cookie to Ogre Root initialization to catch obvious ABI errors. Each library could call checkAbiCookie on initialization to avoid problems
 - Default to clang's ld linker on Linux, i.e. `set( CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld" )`
 - Remove `OgreShadowVolumeExtrudeProgram.cpp`
 - Deprecate `SceneManager::setFog`
 - Remove `getShadowCasterVertex`
 - Remove memory allocator stuff (see Ogre 1.x)
 - Remove nedmalloc
 - Typedef SharedPtr to std::shared_ptr (see Ogre 1.x)
