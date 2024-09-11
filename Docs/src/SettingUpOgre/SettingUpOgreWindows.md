# Windows {#SettingUpOgreWindows}

@tableofcontents

# Requirements {#RequirementsWindows}
    * [CMake 3.x](https://cmake.org/download/)
    * Git
    * Visual Studio 2015 - 2022. MinGW may work but we strongly recommend Visual Studio.
    * [DirectX June 2010 SDK](https://www.microsoft.com/en-us/download/details.aspx?id=6812). Optional.
      Needed if you use older Visual Studio versions and want the D3D11 plugin. Also comes with useful tools.
    * Windows 10 SDK. Contains the latest DirectX SDK, thus recommended over the DX June 2010 SDK,
      but you may still want to install the June 2010 SDK for those tools.
    * Windows 7 or higher is highly recommended. For Windows Vista & 7, you need to have the
      [KB2670838 update](https://support.microsoft.com/en-us/kb/2670838) installed.
      **YOUR END USERS NEED THIS UPDATE AS WELL**.
    * Python 3.x is needed to build shaderc dependency for Vulkan.
    * For HW & SW requirements, please visit http://www.ogre3d.org/developers/requirements

@copydoc DownloadingOgreScriptsCommon

# Downloading Ogre {#DownloadingOgreWindows}

@copydoc DownloadingOgreCommon


# Building Dependencies {#BuildingDependenciesWindows}

-# Open CMake GUI.
-# Point "Where is the source code" to C:\\OgreDepsRepo\\; and
   "Where to build the binaries" pointing to "C:\\OgreDepsRepo\\build"
-# Click 'Configure' and select the generator (i.e. if you're using Visual Studio 2015, select Visual Studio 14)
![](images/CMakeWindowsDependencies.png)
-# Answer 'Ok' when asked if you want to create the build directory
-# Sometimes on bleeding edge repos there might be CMake syntax warnings. Ignore them.
-# Now click 'Generate'.
-# Open C:\\OgreDepsRepo\\build\\OGREDEPS.sln under Visual Studio.
   If you have multiple versions of VS, make sure you open it with the one you targeted for in CMake.
-# Build the whole solution; which should build FreeImage, freetype, OIS, zlib and zziplib.
   Make sure to build both Debug and Release
-# Once it's finished, right click on INSTALL project, and click build. THIS IS VERY IMPORTANT.
   It will create the folder structure that Ogre needs. You need to do this both for Debug and Release.
![](images/BuildWindowsDependencies.png)
-# This should've created an SDK folder structure under C:\\OgreDepsRepo\\build\\ogredeps


# Building Ogre {#BuildingOgreWindows}

-# Open CMake GUI.
-# Point "Where is the source code" to C:\\OgreSDK\\; and "Where to build the binaries"
   pointing to "C:\\OgreSDK\\build"
-# Hit Configure and choose the same Visual Studio generator you did for the dependencies.
   It will now complain it can't find the Dependencies.
-# Set OGRE_DEPENDENCIES_DIR to point to C:\\OgreDepsRepo\\build\\ogredeps as in the picture.
   Use the search filter to find the option quickly. Now hit configure again.
![](images/CMakeWindowsOgre.png)
-# *Pro tip:* If you move the generated folder C:\\OgreDepsRepo\\build\\ogredeps to
   C:\\OgreSDK\\Dependencies you won't need the previous step.
-# Click Generate. If you want to build the samples, now tick OGRE_BUILD_SAMPLES2 and click Generate again.
-# Open the solution C:\\OgreSDK\\build\\OGRE.sln and compile.
-# You're done. You can also right click on INSTALL project if you wish to generate the SDK structure
   we provide in our official releases (we are planning on modifying this on the future for Ogre 2.1 though).
-# If you are like me (I don't like using INSTALL feature for my own projects), you can add
   C:\\OgreSDK\\build\\include to your project's include location and C:\\OgreSDK\\build\\lib\\Debug
   (and Release) to your additional library paths. The DLLs will be located in bin\\debug (or release)

You can watch the [YouTube video](https://www.youtube.com/watch?v=-3XwyYQ9OLg) if you prefer video tutorials.

*Note:* If necessary, ensure OGRE_CONFIG_THREAD_PROVIDER is set to none and that OGRE_CONFIG_THREADS is unchecked.
Don't worry, those CMake configs are about the *old threading* model. For more information see the Wiki's FAQ
[I'm confused about threading support in Ogre 2.1](http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Ogre+2.1+FAQ#I_m_confused_about_threading_support_in_Ogre_2.1)


@addtogroup MdInternal
@{

@addtogroup DownloadingOgreScriptsCommon
@ingroup MdInternal
@{

# Automatic download & build scripts

We provide easy-to-use scripts that will download and setup all dependencies for you in a self-contained folder
Download build_ogre_scripts for the branch you need from:

https://www.ogre3d.org/download/sdk/sdk-ogre-next

@}

@addtogroup DownloadingOgreCommon
@ingroup MdInternal
@{
Ogre Next source is located at [Github/OGRECave/ogre-next](https://github.com/OGRECave/ogre-next)

Ogre Next dependencies are located at [Github/OGRECave/ogre-next-deps](https://github.com/OGRECave/ogre-next-deps)

Download the source code or you can fork it to your own repository and clone it locally with [Github Desktop](https://desktop.github.com/) or the command line.

**IMPORTANT:** Do NOT use the the "Download" feature from Github. It will not follow subrepositories.

**Command line**:
```sh
git clone --recurse-submodules --shallow-submodules https://github.com/OGRECave/ogre-next-deps
git clone --branch master https://github.com/OGRECave/ogre-next
```
@}

@}
