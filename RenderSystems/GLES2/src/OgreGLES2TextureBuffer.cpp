/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
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

#include "OgreRoot.h"

#include "OgreGLES2HardwareBufferManager.h"
#include "OgreGLES2HardwarePixelBuffer.h"
#include "OgreGLES2TextureBuffer.h"
#include "OgreGLES2PixelFormat.h"
#include "OgreGLES2FBORenderTexture.h"
#include "OgreGLES2RenderSystem.h"

#include "OgreGLSLESLinkProgramManager.h"
#include "OgreGLSLESLinkProgram.h"
#include "OgreGLSLESProgramPipelineManager.h"
#include "OgreGLSLESProgramPipeline.h"

namespace Ogre {
namespace v1 {

    GLES2TextureBuffer::GLES2TextureBuffer(const String &baseName, GLenum target, GLuint id, 
                                           GLint width, GLint height, GLint depth, GLint internalFormat, GLint format,
                                           GLint face, GLint level, Usage usage, bool crappyCard, 
                                           bool writeGamma, uint fsaa)
    : GLES2HardwarePixelBuffer(0, 0, 0, PF_UNKNOWN, writeGamma, usage),
        mTarget(target), mTextureID(id), mBufferId(0), mFace(face), mLevel(level), mSoftwareMipmap(crappyCard)
    {
        OGRE_CHECK_GL_ERROR(glBindTexture(mTarget, mTextureID));
        
        // Get face identifier
        mFaceTarget = mTarget;
        if(mTarget == GL_TEXTURE_CUBE_MAP)
            mFaceTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;

        // Calculate the width and height of the texture at this mip level
        mWidth = mLevel == 0 ? width : width / static_cast<size_t>(pow(2.0f, level));
        mHeight = mLevel == 0 ? height : height / static_cast<size_t>(pow(2.0f, level));
        if(mWidth < 1)
            mWidth = 1;
        if(mHeight < 1)
            mHeight = 1;

#if OGRE_NO_GLES3_SUPPORT == 0
        if(target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY)
            mDepth = 1; // Depth always 1 for non-3D textures
        else
            mDepth = depth;
#else
        // Only 2D is supported so depth is always 1
        mDepth = 1;
#endif

        mGLInternalFormat = internalFormat;
        mFormat = GLES2PixelUtil::getClosestOGREFormat(internalFormat, format);

        mRowPitch = mWidth;
        mSlicePitch = mHeight*mWidth;
        mSizeInBytes = PixelUtil::getMemorySize(mWidth, mHeight, mDepth, mFormat);

#if OGRE_DEBUG_MODE
        // Log a message
        std::stringstream str;
        str << "GLES2HardwarePixelBuffer constructed for texture " << baseName
            << " id " << mTextureID << " face " << mFace << " level " << mLevel << ":"
            << " width=" << mWidth << " height="<< mHeight << " depth=" << mDepth
            << " format=" << PixelUtil::getFormatName(mFormat);
        LogManager::getSingleton().logMessage(LML_NORMAL, str.str());
#endif

        // Set up a pixel box
        mBuffer = PixelBox(mWidth, mHeight, mDepth, mFormat);
        
        if (mWidth==0 || mHeight==0 || mDepth==0)
            // We are invalid, do not allocate a buffer
            return;

        // Is this a render target?
        if (mUsage & TU_RENDERTARGET)
        {
            // Create render target for each slice
            mSliceTRT.reserve(mDepth);
            for(uint32 zoffset=0; zoffset<mDepth; ++zoffset)
            {
                String name;
                name = "rtt/" + StringConverter::toString((size_t)this) + "/" + baseName;
                GLES2SurfaceDesc surface;
                surface.buffer = this;
                surface.zoffset = zoffset;
                RenderTexture *trt = GLES2RTTManager::getSingleton().createRenderTexture(name, surface, writeGamma, fsaa);
                mSliceTRT.push_back(trt);
                Root::getSingleton().getRenderSystem()->attachRenderTarget(*mSliceTRT[zoffset]);
            }
        }
    }

    GLES2TextureBuffer::~GLES2TextureBuffer()
    {
        if (mUsage & TU_RENDERTARGET)
        {
            // Delete all render targets that are not yet deleted via _clearSliceRTT because the rendertarget
            // was deleted by the user.
            for (SliceTRT::const_iterator it = mSliceTRT.begin(); it != mSliceTRT.end(); ++it)
            {
                Root::getSingleton().getRenderSystem()->destroyRenderTarget((*it)->getName());
            }
        }
    }

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
    void GLES2TextureBuffer::updateTextureId(GLuint textureID)
    {
        mTextureID = textureID;
    }
#endif
    
#if OGRE_NO_GLES3_SUPPORT == 0
    void GLES2TextureBuffer::upload(const PixelBox &data, const Box &dest)
    {
        GLES2RenderSystem* rs = getGLES2RenderSystem();

        OGRE_CHECK_GL_ERROR(glBindTexture(mTarget, mTextureID));

        OGRE_CHECK_GL_ERROR(glGenBuffers(1, &mBufferId));

        // Use PBO as a texture buffer.
        OGRE_CHECK_GL_ERROR(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mBufferId));

        if(rs->checkExtension("GL_EXT_debug_label"))
        {
            OGRE_CHECK_GL_ERROR(glLabelObjectEXT(GL_BUFFER_OBJECT_EXT, mBufferId, 0, ("Pixel Buffer #" + StringConverter::toString(mBufferId)).c_str()));
        }

        // Calculate size for all mip levels of the texture
        size_t dataSize = 0;
        if(mTarget == GL_TEXTURE_2D_ARRAY)
        {
            dataSize = PixelUtil::getMemorySize(dest.getWidth(), dest.getHeight(), dest.getDepth(), data.format);
        }
        else
        {
            dataSize = PixelUtil::getMemorySize(data.getWidth(), data.getHeight(), mDepth, data.format);
        }
        OGRE_CHECK_GL_ERROR(glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, NULL,
                                         GLES2HardwareBufferManager::getGLUsage(mUsage)));

#if OGRE_DEBUG_MODE
        std::stringstream str;
        str << "GLES2TextureBuffer::upload: " << mTextureID
        << " pixel buffer: " << mBufferId
        << " bytes: " << mSizeInBytes
        << " dest depth: " << dest.getDepth()
        << " dest front: " << dest.front
        << " datasize: " << dataSize
        << " face: " << mFace << " level: " << mLevel
        << " width: " << mWidth << " height: "<< mHeight << " depth: " << mDepth
        << " format: " << PixelUtil::getFormatName(mFormat)
        << " data format: " << PixelUtil::getFormatName(data.format);
        LogManager::getSingleton().logMessage(LML_NORMAL, str.str());
#endif
        void* pBuffer = 0;
        OGRE_CHECK_GL_ERROR(pBuffer = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_RANGE_BIT));

        if(pBuffer == 0)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                        "Texture Buffer: Out of memory",
                        "GLES2TextureBuffer::upload");
        }

        // Copy to destination buffer
        memcpy(pBuffer, data.data, dataSize);
        GLboolean mapped = false;
        OGRE_CHECK_GL_ERROR(mapped = glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
        if(!mapped)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                        "Buffer data corrupted, please reload",
                        "GLES2TextureBuffer::upload");
        }

        if (PixelUtil::isCompressed(data.format))
        {
            if(data.format != mFormat || !data.isConsecutive())
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                            "Compressed images must be consecutive, in the source format",
                            "GLES2TextureBuffer::upload");
            
            GLenum format = GLES2PixelUtil::getClosestGLInternalFormat(mFormat, mHwGamma);
            // Data must be consecutive and at beginning of buffer as PixelStorei not allowed
            // for compressed formats
            switch(mTarget) {
                case GL_TEXTURE_2D:
                case GL_TEXTURE_CUBE_MAP:
                        OGRE_CHECK_GL_ERROR(glCompressedTexSubImage2D(mFaceTarget, mLevel,
                                                  dest.left, dest.top,
                                                  dest.getWidth(), dest.getHeight(),
                                                  format, data.getConsecutiveSize(),
                                                  NULL));
                    break;
                case GL_TEXTURE_3D:
                case GL_TEXTURE_2D_ARRAY:
                    OGRE_CHECK_GL_ERROR(glCompressedTexSubImage3D(mTarget, mLevel,
                                              dest.left, dest.top, dest.front,
                                              dest.getWidth(), dest.getHeight(), dest.getDepth(),
                                              format, data.getConsecutiveSize(),
                                              NULL));
                    break;
            }

        } 
        else
        {
            if(data.getWidth() != data.rowPitch)
                OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ROW_LENGTH, data.rowPitch));
            if(data.getHeight()*data.getWidth() != data.slicePitch)
                OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, (data.slicePitch/data.getWidth())));
            if(data.left > 0 || data.top > 0 || data.front > 0)
                OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_SKIP_PIXELS, data.left + data.rowPitch * data.top + data.slicePitch * data.front));
            if((data.getWidth()*PixelUtil::getNumElemBytes(data.format)) & 3) {
                // Standard alignment of 4 is not right
                OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
            }
#if OGRE_DEBUG_MODE
            LogManager::getSingleton().logMessage("GLES2TextureBuffer::upload - ID: " + StringConverter::toString(mTextureID) +
                                                  " Target: " + StringConverter::toString(mTarget) +
                                                  " Format: " + PixelUtil::getFormatName(data.format) +
                                                  " Origin format: " + StringConverter::toString(GLES2PixelUtil::getGLOriginFormat(data.format, mHwGamma), 0, ' ', std::ios::hex) +
                                                  " Data type: " + StringConverter::toString(GLES2PixelUtil::getGLOriginDataType(data.format), 0, ' ', std::ios::hex));
#endif
            switch(mTarget) {
                case GL_TEXTURE_2D:
                case GL_TEXTURE_CUBE_MAP:
                    OGRE_CHECK_GL_ERROR(glTexSubImage2D(mFaceTarget, mLevel, 
                                    dest.left, dest.top,
                                    dest.getWidth(), dest.getHeight(),
                                    GLES2PixelUtil::getGLOriginFormat(data.format, false), GLES2PixelUtil::getGLOriginDataType(data.format),
                                    NULL));
                    break;
                case GL_TEXTURE_3D:
                case GL_TEXTURE_2D_ARRAY:
                    OGRE_CHECK_GL_ERROR(glTexSubImage3D(
                                    mTarget, mLevel, 
                                    dest.left, dest.top, dest.front,
                                    dest.getWidth(), dest.getHeight(), dest.getDepth(),
                                    GLES2PixelUtil::getGLOriginFormat(data.format, false), GLES2PixelUtil::getGLOriginDataType(data.format),
                                    NULL));
                    break;
            }   
            if (mUsage & TU_AUTOMIPMAP && (mTarget == GL_TEXTURE_2D_ARRAY || mTarget == GL_TEXTURE_3D))
            {
                OGRE_CHECK_GL_ERROR(glGenerateMipmap(mTarget));
            }
        }

        // Delete PBO
        OGRE_CHECK_GL_ERROR(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        OGRE_CHECK_GL_ERROR(glDeleteBuffers(1, &mBufferId));
        mBufferId = 0;

        // Restore defaults
        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0));
        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
    }
    
    //-----------------------------------------------------------------------------  
    void GLES2TextureBuffer::download(const PixelBox &data)
    {
        if(data.getWidth() != getWidth() ||
           data.getHeight() != getHeight() ||
           data.getDepth() != getDepth())
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "only download of entire buffer is supported by GL",
                        "GLES2TextureBuffer::download");

        // Upload data to PBO
        OGRE_CHECK_GL_ERROR(glGenBuffers(1, &mBufferId));
        OGRE_CHECK_GL_ERROR(glBindBuffer(GL_PIXEL_PACK_BUFFER, mBufferId));

        GLES2RenderSystem* rs = getGLES2RenderSystem();
        if(rs->checkExtension("GL_EXT_debug_label"))
        {
            OGRE_CHECK_GL_ERROR(glLabelObjectEXT(GL_BUFFER_OBJECT_EXT, mBufferId, 0, ("Pixel Buffer #" + StringConverter::toString(mBufferId)).c_str()));
        }

        OGRE_CHECK_GL_ERROR(glBufferData(GL_PIXEL_PACK_BUFFER, mSizeInBytes, NULL,
                                         GLES2HardwareBufferManager::getGLUsage(mUsage)));

#if OGRE_DEBUG_MODE
        std::stringstream str;
        str << "GLES2TextureBuffer::download: " << mTextureID
        << " pixel buffer: " << mBufferId
        << " bytes: " << mSizeInBytes
        << " face: " << mFace << " level: " << mLevel
        << " width: " << mWidth << " height: "<< mHeight << " depth: " << mDepth
        << " format: " << PixelUtil::getFormatName(mFormat);
        LogManager::getSingleton().logMessage(LML_NORMAL, str.str());
#endif
        OGRE_CHECK_GL_ERROR(glBindTexture(mTarget, mTextureID));
        if(PixelUtil::isCompressed(data.format))
        {
            if(data.format != mFormat || !data.isConsecutive())
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
                            "Compressed images must be consecutive, in the source format",
                            "GLES2TextureBuffer::download");
        }
        else
        {
            if(data.getWidth() != data.rowPitch)
                OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ROW_LENGTH, (GLint)data.rowPitch));
            if(data.left > 0 || data.top > 0 || data.front > 0)
                OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_SKIP_PIXELS, (GLint)(data.left + data.rowPitch * data.top + data.slicePitch * data.front)));
            if((data.getWidth()*PixelUtil::getNumElemBytes(data.format)) & 3) {
                // Standard alignment of 4 is not right
                OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
            }

            // Restore defaults
            OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ROW_LENGTH, 0));
            OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_SKIP_PIXELS, 0));
            OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 4));
        }

        GLint offsetInBytes = 0;
        size_t width = mWidth;
        size_t height = mHeight;
        size_t depth = mDepth;
        for(GLint i = 0; i < mLevel; i++)
        {
            offsetInBytes += PixelUtil::getMemorySize(width, height, depth, data.format);

            if(width > 1)
                width = width / 2;
            if(height > 1)
                height = height / 2;
            if(depth > 1)
                depth = depth / 2;
        }

        void* pBuffer;
        OGRE_CHECK_GL_ERROR(pBuffer = glMapBufferRange(GL_PIXEL_PACK_BUFFER, offsetInBytes, mSizeInBytes, GL_MAP_READ_BIT));

        if(pBuffer == 0)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                        "Texture Buffer: Out of memory",
                        "GLES2TextureBuffer::download");
        }

        // Copy to destination buffer
        memcpy(data.data, pBuffer, mSizeInBytes);

        GLboolean mapped;
        OGRE_CHECK_GL_ERROR(mapped = glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
        if(!mapped)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                        "Buffer data corrupted, please reload",
                        "GLES2TextureBuffer::download");
        }

        // Delete PBO
        OGRE_CHECK_GL_ERROR(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        OGRE_CHECK_GL_ERROR(glDeleteBuffers(1, &mBufferId));
        mBufferId = 0;
    }
#else
    void GLES2TextureBuffer::upload(const PixelBox &data, const Box &dest)
    {
#if OGRE_DEBUG_MODE
        LogManager::getSingleton().logMessage("GLES2TextureBuffer::upload - ID: " + StringConverter::toString(mTextureID) +
                                              " Target: " + StringConverter::toString(mTarget) +
                                              " Format: " + PixelUtil::getFormatName(data.format) +
                                              " Origin format: " + StringConverter::toString(GLES2PixelUtil::getGLOriginFormat(data.format, mHwGamma), 0, ' ', std::ios::hex) +
                                              " Data type: " + StringConverter::toString(GLES2PixelUtil::getGLOriginDataType(data.format), 0, ' ', std::ios::hex));
#endif
        OGRE_CHECK_GL_ERROR(glBindTexture(mTarget, mTextureID));

        if (PixelUtil::isCompressed(data.format))
        {
            if(data.format != mFormat || !data.isConsecutive())
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                            "Compressed images must be consecutive, in the source format",
                            "GLES2TextureBuffer::upload");

            GLenum format = GLES2PixelUtil::getClosestGLInternalFormat(mFormat, mHwGamma);
            // Data must be consecutive and at beginning of buffer as PixelStorei not allowed
            // for compressed formats
            if (dest.left == 0 && dest.top == 0)
            {
                OGRE_CHECK_GL_ERROR(glCompressedTexImage2D(mFaceTarget, mLevel,
                                       format,
                                       dest.getWidth(),
                                       dest.getHeight(),
                                       0,
                                       static_cast<GLsizei>(data.getConsecutiveSize()),
                                       data.data));
            }
            else
            {
                OGRE_CHECK_GL_ERROR(glCompressedTexSubImage2D(mFaceTarget, mLevel,
                                          dest.left, dest.top,
                                          dest.getWidth(), dest.getHeight(),
                                          format, static_cast<GLsizei>(data.getConsecutiveSize()),
                                          data.data));
            }
        }
        else if (mSoftwareMipmap)
        {
            if (data.getWidth() != data.rowPitch)
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                            "Unsupported texture format",
                            "GLES2TextureBuffer::upload");
            }

            if (data.getHeight() * data.getWidth() != data.slicePitch)
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                            "Unsupported texture format",
                            "GLES2TextureBuffer::upload");
            }

            if ((data.getWidth() * PixelUtil::getNumElemBytes(data.format)) & 3)
            {
                // Standard alignment of 4 is not right
                OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
            }

            buildMipmaps(data);
        }
        else
        {
            if (data.getWidth() != data.rowPitch)
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                            "Unsupported texture format",
                            "GLES2TextureBuffer::upload");
            }

            if (data.getHeight() * data.getWidth() != data.slicePitch)
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                            "Unsupported texture format",
                            "GLES2TextureBuffer::upload");
            }

            if ((data.getWidth() * PixelUtil::getNumElemBytes(data.format)) & 3)
            {
                // Standard alignment of 4 is not right
                OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
            }

            switch(mTarget)
            {
                case GL_TEXTURE_2D:
                case GL_TEXTURE_CUBE_MAP:
                    OGRE_CHECK_GL_ERROR(glTexSubImage2D(mFaceTarget,
                                    mLevel,
                                    dest.left, dest.top,
                                    dest.getWidth(), dest.getHeight(),
                                    //HW Gamma off, we want to pass the data as RGB, not sRGB
                                    GLES2PixelUtil::getGLOriginFormat(data.format, false),
                                    GLES2PixelUtil::getGLOriginDataType(data.format),
                                    data.data));
                break;
            }
        }

        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
    }

    //-----------------------------------------------------------------------------  
    void GLES2TextureBuffer::download(const PixelBox &data)
    {
        if(data.getWidth() != getWidth() ||
           data.getHeight() != getHeight() ||
           data.getDepth() != getDepth())
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "only download of entire buffer is supported by GL ES",
                        "GLES2TextureBuffer::download");

        if(PixelUtil::isCompressed(data.format))
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Compressed images cannot be downloaded by GL ES",
                        "GLES2TextureBuffer::download");
        }

        if((data.getWidth()*PixelUtil::getNumElemBytes(data.format)) & 3) {
            // Standard alignment of 4 is not right
            OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
        }

        GLint currentFBO = 0;
        GLuint tempFBO = 0;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO));
        OGRE_CHECK_GL_ERROR(glGenFramebuffers(1, &tempFBO));
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, tempFBO));

        // Construct a temp PixelBox that is RGBA because GL_RGBA/GL_UNSIGNED_BYTE is the only combination that is
        // guaranteed to work on all platforms.
        size_t sizeInBytes = PixelUtil::getMemorySize(data.getWidth(), data.getHeight(), data.getDepth(), PF_A8B8G8R8);
        PixelBox tempBox = PixelBox(data.getWidth(), data.getHeight(), data.getDepth(), PF_A8B8G8R8);
        tempBox.data = new uint8[sizeInBytes];

        switch (mTarget)
        {
            case GL_TEXTURE_2D:
            case GL_TEXTURE_CUBE_MAP:
                OGRE_CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureID, 0));
                OGRE_CHECK_GL_ERROR(glCheckFramebufferStatus(GL_FRAMEBUFFER));
                OGRE_CHECK_GL_ERROR(glReadPixels(0, 0, data.getWidth(), data.getHeight(),
                                                 GL_RGBA,
                                                 GL_UNSIGNED_BYTE,
                                                 tempBox.data));
                break;
        }

        PixelUtil::bulkPixelConversion(tempBox, data);

        delete[] (uint8*) tempBox.data;
        tempBox.data = 0;

        // Restore defaults
        OGRE_CHECK_GL_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 4));
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, currentFBO));
        OGRE_CHECK_GL_ERROR(glDeleteFramebuffers(1, &tempFBO));
    }
#endif
    //-----------------------------------------------------------------------------  
    void GLES2TextureBuffer::bindToFramebuffer(GLenum attachment, uint32 zoffset)
    {
        assert(zoffset < mDepth);
        OGRE_CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
                                                   mFaceTarget, mTextureID, mLevel));
    }
    
    void GLES2TextureBuffer::copyFromFramebuffer(uint32 zoffset)
    {
        OGRE_CHECK_GL_ERROR(glBindTexture(mTarget, mTextureID));
        OGRE_CHECK_GL_ERROR(glCopyTexSubImage2D(mFaceTarget, mLevel, 0, 0, 0, 0, mWidth, mHeight));
    }

    //-----------------------------------------------------------------------------  
    void GLES2TextureBuffer::blit(const HardwarePixelBufferSharedPtr &src, const Box &srcBox, const Box &dstBox)
    {
        GLES2TextureBuffer *srct = static_cast<GLES2TextureBuffer *>(src.get());
        // TODO: Check for FBO support first
        // Destination texture must be 2D or Cube
        // Source texture must be 2D
        if(((src->getUsage() & TU_RENDERTARGET) == 0 && (srct->mTarget == GL_TEXTURE_2D))
#if OGRE_NO_GLES3_SUPPORT == 0
        || ((srct->mTarget == GL_TEXTURE_3D) && (mTarget != GL_TEXTURE_2D_ARRAY))
#endif
        )
        {
            blitFromTexture(srct, srcBox, dstBox);
        }
        else
        {
            GLES2HardwarePixelBuffer::blit(src, srcBox, dstBox);
        }
    }
    
    //-----------------------------------------------------------------------------  
    // Very fast texture-to-texture blitter and hardware bi/trilinear scaling implementation using FBO
    // Destination texture must be 1D, 2D, 3D, or Cube
    // Source texture must be 1D, 2D or 3D
    // Supports compressed formats as both source and destination format, it will use the hardware DXT compressor
    // if available.
    // @author W.J. van der Laan
    void GLES2TextureBuffer::blitFromTexture(GLES2TextureBuffer *src, const Box &srcBox, const Box &dstBox)
    {
        return; // todo - add a shader attach...
//        std::cerr << "GLES2TextureBuffer::blitFromTexture " <<
//        src->mTextureID << ":" << srcBox.left << "," << srcBox.top << "," << srcBox.right << "," << srcBox.bottom << " " << 
//        mTextureID << ":" << dstBox.left << "," << dstBox.top << "," << dstBox.right << "," << dstBox.bottom << std::endl;

        // Store reference to FBO manager
        GLES2FBOManager *fboMan = static_cast<GLES2FBOManager *>(GLES2RTTManager::getSingletonPtr());
        
        RenderSystem* rs = Root::getSingleton().getRenderSystem();
        rs->_disableTextureUnitsFrom(0);
        glActiveTexture(GL_TEXTURE0);

        // Disable alpha, depth and scissor testing, disable blending, 
        // and disable culling
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);

        // Set up source texture
        OGRE_CHECK_GL_ERROR(glBindTexture(src->mTarget, src->mTextureID));

        // Set filtering modes depending on the dimensions and source
        if(srcBox.getWidth() == dstBox.getWidth() &&
           srcBox.getHeight() == dstBox.getHeight() &&
           srcBox.getDepth() == dstBox.getDepth())
        {
            // Dimensions match -- use nearest filtering (fastest and pixel correct)
            if(src->mUsage & TU_AUTOMIPMAP)
            {
                OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST));
            }
            else
            {
                OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            }
            OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        }
        else
        {
            // Dimensions don't match -- use bi or trilinear filtering depending on the
            // source texture.
            if(src->mUsage & TU_AUTOMIPMAP)
            {
                // Automatic mipmaps, we can safely use trilinear filter which
                // brings greatly improved quality for minimisation.
                OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
            }
            else
            {
                // Manual mipmaps, stay safe with bilinear filtering so that no
                // intermipmap leakage occurs.
                OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            }
            OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        }
        // Clamp to edge (fastest)
        OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
#if OGRE_NO_GLES3_SUPPORT == 0
        OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

        // Set origin base level mipmap to make sure we source from the right mip
        // level.
        OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_BASE_LEVEL, src->mLevel));
#endif
        // Store old binding so it can be restored later
        GLint oldfb;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldfb));

        // Set up temporary FBO
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fboMan->getTemporaryFBO(0)));

        GLuint tempTex = 0;
        if(!fboMan->checkFormat(mFormat))
        {
            // If target format not directly supported, create intermediate texture
            GLenum tempFormat = GLES2PixelUtil::getClosestGLInternalFormat(fboMan->getSupportedAlternative(mFormat), mHwGamma);
            OGRE_CHECK_GL_ERROR(glGenTextures(1, &tempTex));
            OGRE_CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, tempTex));

            GLES2RenderSystem* rs = getGLES2RenderSystem();
            if(rs->hasMinGLVersion(3, 0) || rs->checkExtension("GL_APPLE_texture_max_level"))
                OGRE_CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_APPLE, 0));

            // Allocate temporary texture of the size of the destination area
            OGRE_CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, tempFormat, 
                         GLES2PixelUtil::optionalPO2(dstBox.getWidth()), GLES2PixelUtil::optionalPO2(dstBox.getHeight()), 
                         0, GL_RGBA, GL_UNSIGNED_BYTE, 0));
            OGRE_CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D, tempTex, 0));
            // Set viewport to size of destination slice
            OGRE_CHECK_GL_ERROR(glViewport(0, 0, dstBox.getWidth(), dstBox.getHeight()));
        }
        else
        {
            // We are going to bind directly, so set viewport to size and position of destination slice
            OGRE_CHECK_GL_ERROR(glViewport(dstBox.left, dstBox.top, dstBox.getWidth(), dstBox.getHeight()));
        }
        
        // Process each destination slice
        for(size_t slice = dstBox.front; slice < dstBox.back; ++slice)
        {
            if(!tempTex)
            {
                // Bind directly
                bindToFramebuffer(GL_COLOR_ATTACHMENT0, slice);
            }

            // Calculate source texture coordinates
            float u1 = (float)srcBox.left / (float)src->mWidth;
            float v1 = (float)srcBox.top / (float)src->mHeight;
            float u2 = (float)srcBox.right / (float)src->mWidth;
            float v2 = (float)srcBox.bottom / (float)src->mHeight;
            // Calculate source slice for this destination slice
            float w = (float)(slice - dstBox.front) / (float)dstBox.getDepth();
            // Get slice # in source
            w = w * (float)srcBox.getDepth() + srcBox.front;
            // Normalise to texture coordinate in 0.0 .. 1.0
            w = (w+0.5f) / (float)src->mDepth;
            
            // Finally we're ready to rumble    
            OGRE_CHECK_GL_ERROR(glBindTexture(src->mTarget, src->mTextureID));
            OGRE_CHECK_GL_ERROR(glEnable(src->mTarget));

            GLfloat squareVertices[] = {
               -1.0f, -1.0f,
                1.0f, -1.0f,
               -1.0f,  1.0f,
                1.0f,  1.0f,
            };
            GLfloat texCoords[] = {
                u1, v1, w,
                u2, v1, w,
                u2, v2, w,
                u1, v2, w
            };

            GLuint posAttrIndex = 0;
            GLuint texAttrIndex = 0;
            if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            {
                GLSLESProgramPipeline* programPipeline = GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
                posAttrIndex = (GLuint)programPipeline->getAttributeIndex(VES_POSITION, 0);
                texAttrIndex = (GLuint)programPipeline->getAttributeIndex(VES_TEXTURE_COORDINATES, 0);
            }
            else
            {
                GLSLESLinkProgram* linkProgram = GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();
                posAttrIndex = (GLuint)linkProgram->getAttributeIndex(VES_POSITION, 0);
                texAttrIndex = (GLuint)linkProgram->getAttributeIndex(VES_TEXTURE_COORDINATES, 0);
            }

            // Draw the textured quad
            OGRE_CHECK_GL_ERROR(glVertexAttribPointer(posAttrIndex,
                                  2,
                                  GL_FLOAT,
                                  0,
                                  0,
                                  squareVertices));
            OGRE_CHECK_GL_ERROR(glEnableVertexAttribArray(posAttrIndex));
            OGRE_CHECK_GL_ERROR(glVertexAttribPointer(texAttrIndex,
                                  3,
                                  GL_FLOAT,
                                  0,
                                  0,
                                  texCoords));
            OGRE_CHECK_GL_ERROR(glEnableVertexAttribArray(texAttrIndex));

            OGRE_CHECK_GL_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

            OGRE_CHECK_GL_ERROR(glDisable(src->mTarget));

            if(tempTex)
            {
                // Copy temporary texture
                OGRE_CHECK_GL_ERROR(glBindTexture(mTarget, mTextureID));
                switch(mTarget)
                {
                    case GL_TEXTURE_2D:
                    case GL_TEXTURE_CUBE_MAP:
                        OGRE_CHECK_GL_ERROR(glCopyTexSubImage2D(mFaceTarget, mLevel, 
                                            dstBox.left, dstBox.top, 
                                            0, 0, dstBox.getWidth(), dstBox.getHeight()));
                        break;
                }
            }
        }
        // Finish up 
        if(!tempTex)
        {
            // Generate mipmaps
            if(mUsage & TU_AUTOMIPMAP)
            {
                OGRE_CHECK_GL_ERROR(glBindTexture(mTarget, mTextureID));
                OGRE_CHECK_GL_ERROR(glGenerateMipmap(mTarget));
            }
        }
        
        // Reset source texture to sane state
        OGRE_CHECK_GL_ERROR(glBindTexture(src->mTarget, src->mTextureID));

#if OGRE_NO_GLES3_SUPPORT == 0
        OGRE_CHECK_GL_ERROR(glTexParameteri(src->mTarget, GL_TEXTURE_BASE_LEVEL, 0));

#endif
        OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                          GL_RENDERBUFFER, 0));

        // Restore old framebuffer
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, oldfb));
        OGRE_CHECK_GL_ERROR(glDeleteTextures(1, &tempTex));
    }
    //-----------------------------------------------------------------------------  
    // blitFromMemory doing hardware trilinear scaling
    void GLES2TextureBuffer::blitFromMemory(const PixelBox &src_orig, const Box &dstBox)
    {
        // Fall back to normal GLHardwarePixelBuffer::blitFromMemory in case 
        // - FBO is not supported
        // - Either source or target is luminance due doesn't looks like supported by hardware
        // - the source dimensions match the destination ones, in which case no scaling is needed
        // TODO: Check that extension is NOT available
        if(PixelUtil::isLuminance(src_orig.format) ||
           PixelUtil::isLuminance(mFormat) ||
           (src_orig.getWidth() == dstBox.getWidth() &&
            src_orig.getHeight() == dstBox.getHeight() &&
            src_orig.getDepth() == dstBox.getDepth()))
        {
            GLES2HardwarePixelBuffer::blitFromMemory(src_orig, dstBox);
            return;
        }
        if(!mBuffer.contains(dstBox))
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Destination box out of range",
                        "GLES2TextureBuffer::blitFromMemory");
        // For scoped deletion of conversion buffer
        MemoryDataStreamPtr buf;
        PixelBox src;
        
        // First, convert the srcbox to a OpenGL compatible pixel format
        if(GLES2PixelUtil::getGLOriginFormat(src_orig.format, mHwGamma) == 0)
        {
            // Convert to buffer internal format
            buf.bind(OGRE_NEW MemoryDataStream(PixelUtil::getMemorySize(src_orig.getWidth(), src_orig.getHeight(),
                                                                        src_orig.getDepth(), mFormat)));
            src = PixelBox(src_orig.getWidth(), src_orig.getHeight(), src_orig.getDepth(), mFormat, buf->getPtr());
            PixelUtil::bulkPixelConversion(src_orig, src);
        }
        else
        {
            // No conversion needed
            src = src_orig;
        }
        
        // Create temporary texture to store source data
        GLuint id = 0;
        GLenum target =
#if OGRE_NO_GLES3_SUPPORT == 0
        (src.getDepth() != 1) ? GL_TEXTURE_3D :
#endif
            GL_TEXTURE_2D;

        GLsizei width = (GLsizei)GLES2PixelUtil::optionalPO2(src.getWidth());
        GLsizei height = (GLsizei)GLES2PixelUtil::optionalPO2(src.getHeight());
        GLsizei depth = (GLsizei)GLES2PixelUtil::optionalPO2(src.getDepth());
        GLenum format = GLES2PixelUtil::getClosestGLInternalFormat(src.format, mHwGamma);

        // Generate texture name
        OGRE_CHECK_GL_ERROR(glGenTextures(1, &id));
        
        // Set texture type
        OGRE_CHECK_GL_ERROR(glBindTexture(target, id));

        GLES2RenderSystem* rs = getGLES2RenderSystem();
        if(rs->hasMinGLVersion(3, 0) || rs->checkExtension("GL_APPLE_texture_max_level"))
            OGRE_CHECK_GL_ERROR(glTexParameteri(target, GL_TEXTURE_MAX_LEVEL_APPLE, 1000));

        // Allocate texture memory
#if OGRE_NO_GLES3_SUPPORT == 0
        if(src.getDepth() != 1)
        {
            OGRE_CHECK_GL_ERROR(glTexStorage3D(GL_TEXTURE_3D, 1, format, GLsizei(width), GLsizei(height), GLsizei(depth)));
        }
        else
        {
            OGRE_CHECK_GL_ERROR(glTexStorage2D(GL_TEXTURE_2D, 1, format, GLsizei(width), GLsizei(height)));
        }
#else
        GLenum datatype = (GLsizei)GLES2PixelUtil::getGLOriginDataType(src.format);
        OGRE_CHECK_GL_ERROR(glTexImage2D(target, 0, format, width, height, 0, format, datatype, 0));
#endif

        // GL texture buffer
        GLES2TextureBuffer tex(BLANKSTRING, target, id, width, height, depth, format, src.format,
                              0, 0, (Usage)(TU_AUTOMIPMAP|HBU_STATIC_WRITE_ONLY), false, false, 0);
        
        // Upload data to 0,0,0 in temporary texture
        Box tempTarget(0, 0, 0, src.getWidth(), src.getHeight(), src.getDepth());
        tex.upload(src, tempTarget);
        
        // Blit
        blitFromTexture(&tex, tempTarget, dstBox);
        
        // Delete temp texture
        OGRE_CHECK_GL_ERROR(glDeleteTextures(1, &id));
    }
    
    RenderTexture *GLES2TextureBuffer::getRenderTarget(size_t zoffset)
    {
        assert(mUsage & TU_RENDERTARGET);
        assert(zoffset < mDepth);
        return mSliceTRT[zoffset];
    }

    void GLES2TextureBuffer::buildMipmaps(const PixelBox &data)
    {
        GLsizei width;
        GLsizei height;
        GLsizei depth;
        PixelBox scaled;
        scaled.format = data.format;
        scaled.left = 0;
        scaled.right = mWidth;
        scaled.top = 0;
        scaled.bottom = mHeight;
        scaled.front = 0;
        scaled.back = mDepth;
        scaled.setConsecutive();
       
        if (scaled.format == PF_R8G8B8)
        {
            size_t srcSize = PixelUtil::getMemorySize(data.getWidth(), data.getHeight(), data.getDepth(), data.format);
            scaled.format = PF_B8G8R8;
            scaled.data = new uint8[srcSize];
            memcpy(scaled.data, data.data, srcSize);
            PixelUtil::bulkPixelConversion(data, scaled);
        }
        else
        {
            size_t srcSize = scaled.getConsecutiveSize();
            scaled.data = new uint8[srcSize];
            Image::scale(data, scaled);
        }
        GLint maxMips = GLES2PixelUtil::getMaxMipmaps(mWidth, mHeight, mDepth, mFormat);
        GLenum dataType = GLES2PixelUtil::getGLOriginDataType(scaled.format);
        
        for (GLint level = 0; level <= maxMips; ++level)
        {
            width = (GLsizei)scaled.getWidth();
            height = (GLsizei)scaled.getHeight();
            depth = (GLsizei)scaled.getDepth();
            
            switch(mTarget)
            {
                case GL_TEXTURE_2D:
                case GL_TEXTURE_CUBE_MAP:
                    OGRE_CHECK_GL_ERROR(glTexImage2D(mFaceTarget,
                                                     level,
                                                     mGLInternalFormat,
                                                     width, height,
                                                     0,
                                                     mGLInternalFormat,
                                                     dataType,
                                                     scaled.data));
                    break;
#if OGRE_NO_GLES3_SUPPORT == 0
                case GL_TEXTURE_3D:
                case GL_TEXTURE_2D_ARRAY:
                    OGRE_CHECK_GL_ERROR(glTexImage3D(mFaceTarget,
                                                     level,
                                                     mGLInternalFormat,
                                                     width, height, depth,
                                                     0,
                                                     mGLInternalFormat,
                                                     dataType,
                                                     scaled.data));
                    break;
#endif
            }
            
            bool squashX = (width > 1);
            bool squashY = (height > 1);
#if OGRE_NO_GLES3_SUPPORT == 0
            bool squashZ = (mTarget == GL_TEXTURE_3D && depth > 1);
#else
            bool squashZ = false;
#endif
            if (squashX || squashY || squashZ)
            {
                size_t xMax = squashX ? width >> 1 : width;
                size_t yMax = squashY ? height >> 1 : height;
                size_t zMax = squashZ ? depth >> 1 : depth;
                PixelBox newBox(xMax, yMax, zMax, scaled.format, scaled.data);
                size_t xMask, yMask, zMask;
                size_t mask = 1;
                if (squashX)
                {
                    xMask = mask;
                    mask <<= 1;
                }
                if (squashY)
                {
                    yMask = mask;
                    mask <<= 1;
                }
                if (squashZ)
                {
                    zMask = mask;
                    mask <<= 1;
                }
                // Generate next mipmap level by averaging together consecutive pixels from this level
                for (size_t z = 0; z < zMax; ++z)
                {
                    for (size_t y = 0; y < yMax; ++y)
                    {
                        for (size_t x = 0; x < xMax; ++x)
                        {
                            ColourValue sum = ColourValue::ZERO;
                            for (size_t i = 0; i < mask; ++i)
                            {
                                sum += scaled.getColourAt(squashX ? (x << 1) | (i & xMask ? 1 : 0) : x,
                                                          squashY ? (y << 1) | (i & yMask ? 1 : 0) : y,
                                                          squashZ ? (z << 1) | (i & zMask ? 1 : 0) : z);
                            }
                            newBox.setColourAt(sum / mask, x, y, z);
                        }
                    }
                }
                
                scaled = newBox;
            }
        }
        
        delete[] (uint8*)scaled.data;
        scaled.data = NULL;
    }
}
}
