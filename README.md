
# OGRE3D (Object-Oriented Graphics Rendering Engine)

Ogre is a 3D graphics rendering engine. Not to be confused with a game engine, which provides Networking, Sound, Physics, etc.
This project provides advanced, efficient modern 3D rendering capabilities.

This is the repository where the 2.x branch is actively developed on.
Active development of the 1.x branch happens in https://github.com/OGRECave/ogre

Both branches are in active development. See [What version to choose?](https://www.ogre3d.org/about/what-version-to-choose) to understand the differences between 1.x and 2.x

Both repositories are compatible for merging, but have been split in separate ways as their
differences have diverged long enough.

| Build | Status (github) |
|-------|-----------------|
| MSVC | [![Build status](https://ci.appveyor.com/api/projects/status/github/OGRECave/ogre-next?branch=master&svg=true)](https://ci.appveyor.com/project/MatiasNGoldberg/ogre-next/branch/master)|

# Dependencies

* [CMake 3.x](https://cmake.org/download/)
* Mercurial. We recommend [TortoiseHg](https://tortoisehg.bitbucket.io/download/index.html)
* For HW & SW requirements, please visit http://www.ogre3d.org/developers/requirements

# Dependencies (Windows)

* Visual Studio 2008 SP1 - 2017 (2019 not tested). MinGW may work but we strongly recommend Visual Studio.
* [DirectX June 2010 SDK](https://www.microsoft.com/en-us/download/details.aspx?id=6812). Optional.
  Needed if you use older Visual Studio versions and want the D3D11 plugin. Also comes with useful tools.
* Windows 10 SDK. Contains the latest DirectX SDK, thus recommended over the DX June 2010 SDK,
  but you may still want to install the June 2010 SDK for those tools.
* Windows 7 or newer is highly recommended. For Windows Vista & 7, you need to have the
  [KB2670838 update](https://support.microsoft.com/en-us/kb/2670838) installed.
  **YOUR END USERS NEED THIS UPDATE AS WELL**.

# Dependencies (Linux)

* Clang >3.5 or GCC >4.0

Debian-based. Run:

```
sudo apt-get install libfreetype6-dev libfreeimage-dev libzzip-dev libxrandr-dev libxaw7-dev freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev doxygen graphviz python-clang-4.0 libsdl2-dev cmake ninja-build hg
```

Arch-based Run:

```
pacman -S freeimage freetype2 libxaw libxrandr mesa zziplib cmake gcc
```

# Quick Start

We provide quick download-build scripts under the [Scripts/BuildScripts/output](Scripts/BuildScripts/output) folder.

You can download all of these scripts [as a compressed 7zip file](https://bitbucket.org/sinbad/ogre/downloads/)

If you're on Linux, make sure to first install the dependencies (i.e. run the sudo apt-get above)

# Download and Building manually

If for some reason you want to do it by hand, there's no script for your platform,
or you want to learn what the scripts are actually doing, see
[Setting Up Ogre](https://ogrecave.github.io/ogre/api/2.1/_setting_up_ogre.html) from the Ogre manual.

# Manual

For more information see the [online manual](https://ogrecave.github.io/ogre/api/2.1/manual.html).
The manual can build on Linux using Doxygen:

```
cd build/Debug
ninja OgreDoc
```

# License

OGRE (www.ogre3d.org) is made available under the MIT License.

Copyright (c) 2000-2015 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
