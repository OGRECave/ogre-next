/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd

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

#include "OgreGLES2RenderTexture.h"
#include "OgreGLES2HardwarePixelBuffer.h"
#include "OgreGLES2TextureBuffer.h"

namespace Ogre {
    template<> GLES2RTTManager* Singleton<GLES2RTTManager>::msSingleton = 0;

    GLES2RTTManager::~GLES2RTTManager()
    {
    }

    MultiRenderTarget* GLES2RTTManager::createMultiRenderTarget(const String & name)
    {
        // TODO: Check rendersystem capabilities before throwing the exception
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
                    "MultiRenderTarget is not supported",
                    "GLES2RTTManager::createMultiRenderTarget");
    }

    PixelFormat GLES2RTTManager::getSupportedAlternative(PixelFormat format)
    {
        if (checkFormat(format))
        {
            return format;
        }

        /// Find first alternative
        PixelComponentType pct = PixelUtil::getComponentType(format);

        switch (pct)
        {
            case PCT_BYTE:
                format = PF_A8R8G8B8;
                break;
            case PCT_SHORT:
                format = PF_SHORT_RGBA;
                break;
            case PCT_FLOAT16:
                format = PF_FLOAT16_RGBA;
                break;
            case PCT_FLOAT32:
                format = PF_FLOAT32_RGBA;
                break;
            case PCT_COUNT:
            default:
                break;
        }

        if (checkFormat(format))
            return format;

        /// If none at all, return to default
        return PF_A8R8G8B8;
    }

    GLES2RenderTexture::GLES2RenderTexture(const String &name,
                                         const GLES2SurfaceDesc &target,
                                         bool writeGamma,
                                         uint fsaa)
        : RenderTexture(target.buffer, target.zoffset)
    {
        mName = name;
        mHwGamma = writeGamma;
        mFSAA = fsaa;
    }

    GLES2RenderTexture::~GLES2RenderTexture()
    {
    }
}
