/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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

#include "OgreGLES2HardwareBufferManager.h"
#include "OgreGLES2HardwarePixelBuffer.h"
#include "OgreGLES2PixelFormat.h"
#include "OgreGLES2FBORenderTexture.h"
#include "OgreGLES2Util.h"
#include "OgreGLES2RenderSystem.h"
#include "OgreRoot.h"
#include "OgreGLSLESLinkProgramManager.h"
#include "OgreGLSLESLinkProgram.h"
#include "OgreGLSLESProgramPipelineManager.h"
#include "OgreGLSLESProgramPipeline.h"
#include "OgreBitwise.h"

namespace Ogre {
namespace v1 {
    GLES2HardwarePixelBuffer::GLES2HardwarePixelBuffer(uint32 width, uint32 height,
                                                     uint32 depth, PixelFormat format, bool hwGamma,
                                                     HardwareBuffer::Usage usage)
        : HardwarePixelBuffer(width, height, depth, format, hwGamma, usage, false, false),
          mBuffer(width, height, depth, format),
          mGLInternalFormat(GL_NONE)
    {
    }

    GLES2HardwarePixelBuffer::~GLES2HardwarePixelBuffer()
    {
        // Force free buffer
        delete [] (uint8*)mBuffer.data;
    }

    void GLES2HardwarePixelBuffer::allocateBuffer()
    {
        if (mBuffer.data)
            // Already allocated
            return;

        mBuffer.data = new uint8[mSizeInBytes];
        // TODO use PBO if we're HBU_DYNAMIC
    }

    void GLES2HardwarePixelBuffer::freeBuffer()
    {
        // Free buffer if we're STATIC to save memory
        if (mUsage & HBU_STATIC)
        {
            delete [] (uint8*)mBuffer.data;
            mBuffer.data = 0;
        }
    }

    PixelBox GLES2HardwarePixelBuffer::lockImpl(const Box &lockBox,  LockOptions options)
    {
        allocateBuffer();
        if (options != HardwareBuffer::HBL_DISCARD)
        {
            // Download the old contents of the texture
            download(mBuffer);
        }
        mCurrentLockOptions = options;
        mLockedBox = lockBox;
        return mBuffer.getSubVolume(lockBox);
    }

    void GLES2HardwarePixelBuffer::unlockImpl(void)
    {
        if (mCurrentLockOptions != HardwareBuffer::HBL_READ_ONLY)
        {
            // From buffer to card, only upload if was locked for writing
            upload(mCurrentLock, mLockedBox);
        }
        freeBuffer();
    }

    void GLES2HardwarePixelBuffer::blitFromMemory(const PixelBox &src, const Box &dstBox)
    {
        if (!mBuffer.contains(dstBox))
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Destination box out of range",
                        "GLES2HardwarePixelBuffer::blitFromMemory");
        }

        bool freeScaledBuffer = false;
        PixelBox scaled;

        if (src.getWidth() != dstBox.getWidth() ||
            src.getHeight() != dstBox.getHeight() ||
            src.getDepth() != dstBox.getDepth())
        {
            // Scale to destination size.
            // This also does pixel format conversion if needed
            allocateBuffer();
            scaled = mBuffer.getSubVolume(dstBox);
            Image::scale(src, scaled, Image::FILTER_BILINEAR);
        }
        else if (GLES2PixelUtil::getGLOriginFormat(src.format, mHwGamma) == 0)
        {
            // Extents match, but format is not accepted as valid source format for GL
            // do conversion in temporary buffer
            allocateBuffer();
            scaled = mBuffer.getSubVolume(dstBox);
            PixelUtil::bulkPixelConversion(src, scaled);
        }
        else
        {
            allocateBuffer();

            // No scaling or conversion needed
            scaled = PixelBox(src.getWidth(), src.getHeight(), src.getDepth(), src.format, src.data);

            if (src.format == PF_R8G8B8)
            {
                freeScaledBuffer = true;
                size_t srcSize = PixelUtil::getMemorySize(src.getWidth(), src.getHeight(), src.getDepth(), src.format);
                scaled.format = PF_B8G8R8;
                scaled.data = new uint8[srcSize];
                memcpy(scaled.data, src.data, srcSize);
                PixelUtil::bulkPixelConversion(src, scaled);
            }
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
            if (src.format == PF_A8R8G8B8)
            {
                scaled.format = PF_A8B8G8R8;
                PixelUtil::bulkPixelConversion(src, scaled);
            }
#endif
        }

        upload(scaled, dstBox);
        freeBuffer();
        
        if (freeScaledBuffer)
        {
            delete[] (uint8*)scaled.data;
        }
    }

    void GLES2HardwarePixelBuffer::blitToMemory(const Box &srcBox, const PixelBox &dst)
    {
        if (!mBuffer.contains(srcBox))
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "source box out of range",
                        "GLES2HardwarePixelBuffer::blitToMemory");
        }

        if (srcBox.left == 0 && srcBox.right == getWidth() &&
            srcBox.top == 0 && srcBox.bottom == getHeight() &&
            srcBox.front == 0 && srcBox.back == getDepth() &&
            dst.getWidth() == getWidth() &&
            dst.getHeight() == getHeight() &&
            dst.getDepth() == getDepth() &&
            GLES2PixelUtil::getGLOriginFormat(dst.format, mHwGamma) != 0)
        {
            // The direct case: the user wants the entire texture in a format supported by GL
            // so we don't need an intermediate buffer
            download(dst);
        }
        else
        {
            // Use buffer for intermediate copy
            allocateBuffer();
            // Download entire buffer
            download(mBuffer);
            if(srcBox.getWidth() != dst.getWidth() ||
                srcBox.getHeight() != dst.getHeight() ||
                srcBox.getDepth() != dst.getDepth())
            {
                // We need scaling
                Image::scale(mBuffer.getSubVolume(srcBox), dst, Image::FILTER_BILINEAR);
            }
            else
            {
                // Just copy the bit that we need
                PixelUtil::bulkPixelConversion(mBuffer.getSubVolume(srcBox), dst);
            }
            freeBuffer();
        }
    }

    void GLES2HardwarePixelBuffer::upload(const PixelBox &data, const Box &dest)
    {
        OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                    "Upload not possible for this pixelbuffer type",
                    "GLES2HardwarePixelBuffer::upload");
    }

    void GLES2HardwarePixelBuffer::download(const PixelBox &data)
    {
        OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                    "Download not possible for this pixelbuffer type",
                    "GLES2HardwarePixelBuffer::download");
    }

    void GLES2HardwarePixelBuffer::bindToFramebuffer(GLenum attachment, uint32 zoffset)
    {
        OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                    "Framebuffer bind not possible for this pixelbuffer type",
                    "GLES2HardwarePixelBuffer::bindToFramebuffer");
    }
    
    
    //********* GLES2RenderBuffer
    //----------------------------------------------------------------------------- 
    GLES2RenderBuffer::GLES2RenderBuffer(GLenum format, uint32 width, uint32 height, GLsizei numSamples):
    GLES2HardwarePixelBuffer(width, height, 1, GLES2PixelUtil::getClosestOGREFormat(format, GL_RGBA), false, HBU_WRITE_ONLY)
    {
        GLES2RenderSystem* rs = getGLES2RenderSystem();

        mGLInternalFormat = format;
        mNumSamples = numSamples;
        
        // Generate renderbuffer
        OGRE_CHECK_GL_ERROR(glGenRenderbuffers(1, &mRenderbufferID));

        // Bind it to FBO
        OGRE_CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, mRenderbufferID));

        if(rs->checkExtension("GL_EXT_debug_label"))
        {
            OGRE_CHECK_GL_ERROR(glLabelObjectEXT(GL_RENDERBUFFER, mRenderbufferID, 0, ("RB " + StringConverter::toString(mRenderbufferID) + " MSAA: " + StringConverter::toString(mNumSamples)).c_str()));
        }

        // Allocate storage for depth buffer
        if (mNumSamples > 0)
        {
            if(rs->hasMinGLVersion(3, 0) || rs->checkExtension("GL_APPLE_framebuffer_multisample"))
            {
                OGRE_CHECK_GL_ERROR(glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER,
                                                                          mNumSamples, mGLInternalFormat, mWidth, mHeight));
            }
        }
        else
        {
            OGRE_CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, mGLInternalFormat,
                                                      mWidth, mHeight));
        }
    }
    //----------------------------------------------------------------------------- 
    GLES2RenderBuffer::~GLES2RenderBuffer()
    {
        OGRE_CHECK_GL_ERROR(glDeleteRenderbuffers(1, &mRenderbufferID));
    }
    //-----------------------------------------------------------------------------  
    void GLES2RenderBuffer::bindToFramebuffer(GLenum attachment, uint32 zoffset)
    {
        assert(zoffset < mDepth);
        OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment,
                                                      GL_RENDERBUFFER, mRenderbufferID));
    }
}
}
