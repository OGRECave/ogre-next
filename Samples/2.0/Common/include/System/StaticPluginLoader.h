/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2016 Torus Knot Software Ltd

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
-----------------------------------------------------------------------------
*/

#ifndef _Demo_StaticPluginLoader_H_
#define _Demo_StaticPluginLoader_H_

#include "OgreBuildSettings.h"

namespace Ogre
{
#ifdef OGRE_STATIC_LIB
#    ifdef OGRE_BUILD_RENDERSYSTEM_METAL
    class MetalPlugin;
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_D3D11
    class D3D11Plugin;
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_GL3PLUS
    class GL3PlusPlugin;
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_GLES2
    class GLES2Plugin;
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_VULKAN
    class VulkanPlugin;
#    endif
#endif
    class Root;
}  // namespace Ogre

namespace Demo
{
    /// Utility class to load plugins statically
    class StaticPluginLoader
    {
#ifdef OGRE_STATIC_LIB
        unsigned int mDummy;
#    ifdef OGRE_BUILD_RENDERSYSTEM_GL3PLUS
        Ogre::GL3PlusPlugin *mGL3PlusPlugin;
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_GLES2
        Ogre::GLES2Plugin *mGLES2Plugin;
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_D3D11
        Ogre::D3D11Plugin *mD3D11PlusPlugin;
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_METAL
        Ogre::MetalPlugin *mMetalPlugin;
#    endif
#    ifdef OGRE_BUILD_RENDERSYSTEM_VULKAN
        Ogre::VulkanPlugin *mVulkanPlugin;
#    endif
#endif
    public:
        StaticPluginLoader();
        ~StaticPluginLoader();

        void install( Ogre::Root *root );
    };
}  // namespace Demo

#endif
