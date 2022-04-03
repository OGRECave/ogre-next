# What's new in Ogre 2.4 {#Ogre24Changes}

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
| OgreMain.dll               | OgreMain.dll                   |
| OgreHlmsPbs.dll            | OgreNextHlmsPbs.dll            |
| OgreHlmsUnlit.dll          | OgreNextHlmsUnlit.dll          |
| OgreMeshLodGenerator.dll   | OgreNextMeshLodGenerator.dll   |
| OgreOverlay.dll            | OgreNextOverlay.dll            |
| OgreSceneFormat.dll        | OgreNextSceneFormat.dll        |
| libOgreMain.so             | libOgreMain.so                 |
| libOgreHlmsPbs.so          | libOgreNextHlmsPbs.so          |
| libOgreHlmsUnlit.so        | libOgreNextHlmsUnlit.so        |
| libOgreMeshLodGenerator.so | libOgreNextMeshLodGenerator.so |
| libOgreOverlay.so          | libOgreNextOverlay.so          |
| libOgreSceneFormat.so      | libOgreNextSceneFormat.so      |

We understand this change can wreak havoc on our users who have scripts expecting to find OgreMain instead of OgreNextMain.

**Which is why this option will be off by default in OgreNext 2.4;
but will be turned on by default in OgreNext 2.5**

**The CMake option is scheduled for removal in OgreNext 2.6**

`EmptyProject`'s and Android's scripts have been updated to autodetect which name is being used and select between Ogre and OgreNext and CMake config time.

Make sure to upgrade to latest CMake scripts if you're using them; to be ready for all changes.