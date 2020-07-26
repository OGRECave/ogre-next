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

#include "OgreGLES2FBORenderTexture.h"
#include "OgreGLES2PixelFormat.h"
#include "OgreLogManager.h"
#include "OgreGLES2HardwarePixelBuffer.h"
#include "OgreGLES2FBOMultiRenderTarget.h"
#include "OgreRoot.h"
#include "OgreGLES2RenderSystem.h"
#include "OgreGLES2Util.h"

namespace Ogre {
    static const size_t TEMP_FBOS = 2;

    GLES2FBORenderTexture::GLES2FBORenderTexture(
        GLES2FBOManager *manager, const String &name,
        const GLES2SurfaceDesc &target, bool writeGamma, uint fsaa):
        GLES2RenderTexture(name, target, writeGamma, fsaa),
        mFB(manager, fsaa)
    {
        // Bind target to surface 0 and initialise
        mFB.bindSurface(0, target);

        // Get attributes
        mWidth = mFB.getWidth();
        mHeight = mFB.getHeight();
    }
    
    void GLES2FBORenderTexture::getCustomAttribute(const String& name, void* pData)
    {
        if(name=="FBO")
        {
            *static_cast<GLES2FrameBufferObject **>(pData) = &mFB;
        }
        else if (name == "GL_FBOID")
        {
            *static_cast<GLuint*>(pData) = mFB.getGLFBOID();
        }
        else if (name == "GL_MULTISAMPLEFBOID")
        {
            *static_cast<GLuint*>(pData) = mFB.getGLMultisampleFBOID();
        }
    }

    void GLES2FBORenderTexture::swapBuffers()
    {
        if( isFsaaResolveDirty() )
            mFB.swapBuffers();
        GLES2RenderTexture::swapBuffers();
    }
    
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
    void GLES2FBORenderTexture::notifyOnContextLost()
    {
        mFB.notifyOnContextLost();
    }
    
    void GLES2FBORenderTexture::notifyOnContextReset()
    {
        GLES2SurfaceDesc target;
        target.buffer = static_cast<GLES2HardwarePixelBuffer*>(mBuffer);
        target.zoffset = mZOffset;
        
        mFB.notifyOnContextReset(target);
        
        static_cast<GLES2RenderSystem*>(Ogre::Root::getSingletonPtr()->getRenderSystem())->_createDepthBufferFor(this);
    }
#endif
    
    //-----------------------------------------------------------------------------
    bool GLES2FBORenderTexture::attachDepthBuffer( DepthBuffer *depthBuffer, bool exactFormatMatch )
    {
        bool result;
        if( (result = GLES2RenderTexture::attachDepthBuffer( depthBuffer, exactFormatMatch)) )
            mFB.attachDepthBuffer( depthBuffer );

        return result;
    }
    //-----------------------------------------------------------------------------
    void GLES2FBORenderTexture::detachDepthBuffer()
    {
        mFB.detachDepthBuffer();
        GLES2RenderTexture::detachDepthBuffer();
    }
    //-----------------------------------------------------------------------------
    void GLES2FBORenderTexture::_detachDepthBuffer()
    {
        mFB.detachDepthBuffer();
        GLES2RenderTexture::_detachDepthBuffer();
    }
   
    // Size of probe texture
    #define PROBE_SIZE 16

    // Stencil and depth formats to be tried
    static const GLenum stencilFormats[] =
    {
        GL_NONE,                    // No stencil
#if OGRE_NO_GLES3_SUPPORT == 1
        GL_STENCIL_INDEX1_OES,
        GL_STENCIL_INDEX4_OES,
#endif
        GL_STENCIL_INDEX8
    };
    static const size_t stencilBits[] =
    {
        0,
#if OGRE_NO_GLES3_SUPPORT == 1
        1,
        4,
#endif
        8
    };
    #define STENCILFORMAT_COUNT (sizeof(stencilFormats)/sizeof(GLenum))

    static const GLenum depthFormats[] =
    {
        GL_NONE,
        GL_DEPTH_COMPONENT16,
        GL_DEPTH_COMPONENT24_OES,   // Prefer 24 bit depth
        GL_DEPTH_COMPONENT32_OES,
        GL_DEPTH24_STENCIL8_OES    // Packed depth / stencil
#if OGRE_NO_GLES3_SUPPORT == 0
        , GL_DEPTH32F_STENCIL8
#endif
    };
    static const size_t depthBits[] =
    {
        0
        ,16
        ,24
        ,32
        ,24
#if OGRE_NO_GLES3_SUPPORT == 0
        ,32
#endif
    };
    #define DEPTHFORMAT_COUNT (sizeof(depthFormats)/sizeof(GLenum))

    GLES2FBOManager::GLES2FBOManager()
    {
        detectFBOFormats();
        
        mTempFBO.resize(Ogre::TEMP_FBOS, 0);

        for (size_t i = 0; i < Ogre::TEMP_FBOS; i++)
        {
            OGRE_CHECK_GL_ERROR(glGenFramebuffers(1, &mTempFBO[i]));
        }
    }

    GLES2FBOManager::~GLES2FBOManager()
    {
        if(!mRenderBufferMap.empty())
        {
            LogManager::getSingleton().logMessage("GL ES 2: Warning! GLES2FBOManager destructor called, but not all renderbuffers were released.", LML_CRITICAL);
        }
        
        for (size_t i = 0; i < Ogre::TEMP_FBOS; i++)
        {
            OGRE_CHECK_GL_ERROR(glDeleteFramebuffers(1, &mTempFBO[i]));
        }
    }
    
    void GLES2FBOManager::_reload()
    {
        for (size_t i = 0; i < Ogre::TEMP_FBOS; i++)
        {
            OGRE_CHECK_GL_ERROR(glDeleteFramebuffers(1, &mTempFBO[i]));
        }
        
        detectFBOFormats();

        for (size_t i = 0; i < Ogre::TEMP_FBOS; i++)
        {
            OGRE_CHECK_GL_ERROR(glGenFramebuffers(1, &mTempFBO[i]));
        }
    }

    void GLES2FBOManager::_createTempFramebuffer(PixelFormat pixFmt, GLuint internalFormat, GLuint fmt, GLenum dataType, GLuint &fb, GLuint &tid)
    {
        OGRE_CHECK_GL_ERROR(glGenFramebuffers(1, &fb));
        OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fb));
        if (fmt != GL_NONE)
        {
            if (tid)
                OGRE_CHECK_GL_ERROR(glDeleteTextures(1, &tid));

            // Create and attach texture
            OGRE_CHECK_GL_ERROR(glGenTextures(1, &tid));
            OGRE_CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, tid));

            // Set some default parameters
            OGRE_CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            OGRE_CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            OGRE_CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            OGRE_CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, PROBE_SIZE, PROBE_SIZE, 0, fmt, dataType, 0);
            
            if( PixelUtil::isDepth( pixFmt ) )
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tid, 0);
            }
            else
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tid, 0);
            }
            
            // Clear errors
            glGetError();
        }
//        else
//        {
//            // Draw to nowhere -- stencil/depth only
//            OGRE_CHECK_GL_ERROR(glDrawBuffer(GL_NONE));
//            OGRE_CHECK_GL_ERROR(glReadBuffer(GL_NONE));
//        }
    }

    /** Try a certain FBO format, and return the status. Also sets mDepthRB and mStencilRB.
        @returns true    if this combo is supported
                 false   if this combo is not supported
    */
    GLuint GLES2FBOManager::_tryFormat(GLenum depthFormat, GLenum stencilFormat)
    {
        GLuint status, depthRB = 0, stencilRB = 0;

        if(depthFormat != GL_NONE)
        {
            // Generate depth renderbuffer
            OGRE_CHECK_GL_ERROR(glGenRenderbuffers(1, &depthRB));

            // Bind it to FBO
            OGRE_CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, depthRB));

            // Allocate storage for depth buffer
            OGRE_CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, depthFormat,
                                PROBE_SIZE, PROBE_SIZE));

            // Attach depth
            OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRB));
        }

        if(stencilFormat != GL_NONE)
        {
            // Generate stencil renderbuffer
            OGRE_CHECK_GL_ERROR(glGenRenderbuffers(1, &stencilRB));

            // Bind it to FBO
            OGRE_CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, stencilRB));

            // Allocate storage for stencil buffer
            OGRE_CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, stencilFormat, PROBE_SIZE, PROBE_SIZE));

            // Attach stencil
            OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, stencilRB));
        }

        OGRE_CHECK_GL_ERROR(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));

        // If status is negative, clean up
        // Detach and destroy
        OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0));
        OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0));

        if (depthRB)
            OGRE_CHECK_GL_ERROR(glDeleteRenderbuffers(1, &depthRB));

        if (stencilRB)
            OGRE_CHECK_GL_ERROR(glDeleteRenderbuffers(1, &stencilRB));
        
        return status == GL_FRAMEBUFFER_COMPLETE;
    }
    
    /** Try a certain packed depth/stencil format, and return the status.
        @return true    if this combo is supported
                false   if this combo is not supported
    */
    bool GLES2FBOManager::_tryPackedFormat(GLenum packedFormat)
    {
        GLuint packedRB;

        // Generate renderbuffer
        OGRE_CHECK_GL_ERROR(glGenRenderbuffers(1, &packedRB));

        // Bind it to FBO
        OGRE_CHECK_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, packedRB));

        // Allocate storage for buffer
        OGRE_CHECK_GL_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, packedFormat, PROBE_SIZE, PROBE_SIZE));

        // Attach depth
        OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER, packedRB));

        // Attach stencil
        OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER, packedRB));

        GLuint status;
        OGRE_CHECK_GL_ERROR(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));

        // Detach and destroy
        OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0));
        OGRE_CHECK_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0));
        OGRE_CHECK_GL_ERROR(glDeleteRenderbuffers(1, &packedRB));

        return status == GL_FRAMEBUFFER_COMPLETE;
    }

    /** Detect which internal formats are allowed as RTT
        Also detect what combinations of stencil and depth are allowed with this internal
        format.
    */
    void GLES2FBOManager::detectFBOFormats()
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
        // TODO: Fix that probing all formats slows down startup not just on the web also on Android / iOS
        for(size_t x = 0; x < PF_COUNT; ++x)
        {
            mProps[x].valid = true;
        }
        LogManager::getSingleton().logMessage("[GLES2] : detectFBOFormats is disabled on this platform (due performance reasons)");
#else
        // Try all formats, and report which ones work as target
        GLES2RenderSystem* rs = getGLES2RenderSystem();
        GLuint fb = 0, tid = 0;
        GLint oldfb = 0;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldfb));

        bool formatSupported;

        for(size_t x = 0; x < PF_COUNT; ++x)
        {
            mProps[x].valid = false;

            // Fetch GL format token
            GLenum internalFormat = GLES2PixelUtil::getGLInternalFormat((PixelFormat)x, false);
            GLenum originFormat = GLES2PixelUtil::getGLOriginFormat((PixelFormat)x, false);
            GLenum dataType = GLES2PixelUtil::getGLOriginDataType((PixelFormat)x);

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
            if(internalFormat == GL_NONE || originFormat == GL_NONE || dataType == GL_NONE)
                continue;
#else
            if((internalFormat == GL_NONE || originFormat == GL_NONE || dataType == GL_NONE) && (x != 0))
                continue;
#endif
            // No test for compressed formats
            if(PixelUtil::isCompressed((PixelFormat)x))
                continue;

            // Create and attach framebuffer
            _createTempFramebuffer((PixelFormat)x, internalFormat, originFormat, dataType, fb, tid);

            // Check status
            GLuint status;
            OGRE_CHECK_GL_ERROR(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
            formatSupported = status == GL_FRAMEBUFFER_COMPLETE;

            // Ignore status in case of fmt==GL_NONE, because no implementation will accept
            // a buffer without *any* attachment. Buffers with only stencil and depth attachment
            // might still be supported, so we must continue probing.
            if(internalFormat == GL_NONE || formatSupported)
            {
                mProps[x].valid = true;
                StringStream str;
                str << "FBO " << PixelUtil::getFormatName((PixelFormat)x) 
                    << " depth/stencil support: ";

                // For each depth/stencil formats
                for (size_t depth = 0; depth < DEPTHFORMAT_COUNT; ++depth)
                {
#if OGRE_NO_GLES3_SUPPORT == 1
                    if (rs->checkExtension("GL_OES_packed_depth_stencil") && depthFormats[depth] != GL_DEPTH24_STENCIL8_OES)
#else
                    if (depthFormats[depth] != GL_DEPTH24_STENCIL8 && depthFormats[depth] != GL_DEPTH32F_STENCIL8)
#endif
                    {
                        // General depth/stencil combination

                        for (size_t stencil = 0; stencil < STENCILFORMAT_COUNT; ++stencil)
                        {
//                            StringStream l;
//                            l << "Trying " << PixelUtil::getFormatName((PixelFormat)x) 
//                              << " D" << depthBits[depth] 
//                              << "S" << stencilBits[stencil];
//                            LogManager::getSingleton().logMessage(l.str());
                            
                            formatSupported = _tryFormat(depthFormats[depth], stencilFormats[stencil]) != 0;
                            if (formatSupported)
                            {
                                // Add mode to allowed modes
                                str << "D" << depthBits[depth] << "S" << stencilBits[stencil] << " ";
                                FormatProperties::Mode mode = { depth, stencil };
                                mProps[x].modes.push_back(mode);
                            }
                            else
                            {
                                // There is a small edge case that FBO is trashed during the test
                                // on some drivers resulting in undefined behavior
                                OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, oldfb));
                                OGRE_CHECK_GL_ERROR(glDeleteFramebuffers(1, &fb));

                                _createTempFramebuffer((PixelFormat)x, internalFormat, originFormat, dataType, fb, tid);
                            }
                        }
                    }
                    else
                    {
                        // Packed depth/stencil format
                        formatSupported = _tryPackedFormat(depthFormats[depth]);
                        if (formatSupported)
                        {
                            // Add mode to allowed modes
                            str << "Packed-D" << depthBits[depth] << "S" << 8 << " ";
                            FormatProperties::Mode mode = { depth, 0 }; // stencil unused
                            mProps[x].modes.push_back(mode);
                        }
                        else
                        {
                            // There is a small edge case that FBO is trashed during the test
                            // on some drivers resulting in undefined behavior
                            OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, oldfb));
                            OGRE_CHECK_GL_ERROR(glDeleteFramebuffers(1, &fb));

                            _createTempFramebuffer((PixelFormat)x, internalFormat, originFormat, dataType, fb, tid);
                        }
                    }
                }
                LogManager::getSingleton().logMessage(str.str());
            }

            // Delete texture and framebuffer
            OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, oldfb));
            OGRE_CHECK_GL_ERROR(glDeleteFramebuffers(1, &fb));
            
            if (internalFormat != GL_NONE) {
                OGRE_CHECK_GL_ERROR(glDeleteTextures(1, &tid));
                tid = 0;
            }
        }

        String fmtstring;
        for(size_t x = 0; x < PF_COUNT; ++x)
        {
            if(mProps[x].valid)
                fmtstring += PixelUtil::getFormatName((PixelFormat)x)+" ";
        }
        LogManager::getSingleton().logMessage("[GLES2] : Valid FBO targets " + fmtstring);
#endif
    }

    void GLES2FBOManager::getBestDepthStencil(PixelFormat depthFormat, PixelFormat fboFormat,
                                              GLenum *outDepthFormat, GLenum *outStencilFormat)
    {
        const FormatProperties &props = mProps[fboFormat];

        GLenum internalDepthFormat = GLES2PixelUtil::getGLInternalFormat( depthFormat, false );

        //Look for exact match, will try to get depth+stencil packed formats.
        for( size_t mode=0; mode<props.modes.size(); ++mode )
        {
            if( depthFormats[props.modes[mode].depth] == internalDepthFormat )
            {
                *outDepthFormat      = depthFormats[props.modes[mode].depth];
                *outStencilFormat    = GL_NONE;
                return;
            }
        }

        //If we reach here, either the format is not supported, or the depth+stencil
        //must not be packed. Look for non-packed formats now.
        for( size_t mode=0; mode<props.modes.size(); ++mode )
        {
            if( (depthFormats[props.modes[mode].depth] == GL_DEPTH_COMPONENT24_OES &&
                stencilFormats[props.modes[mode].stencil] == GL_STENCIL_INDEX8 &&
                    (depthFormat == PF_D24_UNORM_S8_UINT || depthFormat == PF_D24_UNORM_X8 ||
                     depthFormat == PF_X24_S8_UINT)) ||
                (depthFormats[props.modes[mode].depth] == GL_DEPTH_COMPONENT32_OES &&
                stencilFormats[props.modes[mode].stencil] == GL_STENCIL_INDEX8 &&
                    (depthFormat == PF_D32_FLOAT_X24_S8_UINT || depthFormat == PF_D32_FLOAT_X24_X8 ||
                     depthFormat == PF_X32_X24_S8_UINT)) )
            {
                *outDepthFormat      = depthFormats[props.modes[mode].depth];
                *outStencilFormat    = stencilFormats[props.modes[mode].stencil];
                return;
            }
        }

        //If we end here, we couldn't find a compatible format.
        *outDepthFormat      = GL_NONE;
        *outStencilFormat    = GL_NONE;
    }

    GLES2FBORenderTexture *GLES2FBOManager::createRenderTexture(const String &name, 
        const GLES2SurfaceDesc &target, bool writeGamma, uint fsaa)
    {
        GLES2FBORenderTexture *retval = new GLES2FBORenderTexture(this, name, target, writeGamma, fsaa);
        return retval;
    }
    MultiRenderTarget *GLES2FBOManager::createMultiRenderTarget(const String & name)
    {
        return new GLES2FBOMultiRenderTarget(this, name);
    }

    void GLES2FBOManager::bind(RenderTarget *target)
    {
        // Check if the render target is in the rendertarget->FBO map
        GLES2FrameBufferObject *fbo = 0;
        target->getCustomAttribute("FBO", &fbo);
        if(fbo)
            fbo->bind();
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        else
        {
            // Non-multisampled window buffer is FBO #1 on iOS, multisampled is yet another,
            // so give the target ability to influence decision which FBO to use
            GLuint glfbo = 0;
            target->getCustomAttribute("GLFBO", &glfbo);
            if(glfbo != 0) // prevent GLSL validation result : Validation Failed: Current draw framebuffer is invalid.
                OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, glfbo));
        }
#else
        else
            // Old style context (window/pbuffer) or copying render texture
            OGRE_CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
#endif
    }
    
    GLES2SurfaceDesc GLES2FBOManager::requestRenderBuffer(GLenum format, uint32 width, uint32 height, uint fsaa)
    {
        GLES2SurfaceDesc retval;
        retval.buffer = 0; // Return 0 buffer if GL_NONE is requested
        if(format != GL_NONE)
        {
            RBFormat key(format, width, height, fsaa);
            RenderBufferMap::iterator it = mRenderBufferMap.find(key);
            if(it != mRenderBufferMap.end() && (it->second.refcount == 0))
            {
                retval.buffer = it->second.buffer;
                retval.zoffset = 0;
                retval.numSamples = fsaa;
                // Increase refcount
                ++it->second.refcount;
            }
            else
            {
                // New one
                v1::GLES2RenderBuffer *rb = OGRE_NEW v1::GLES2RenderBuffer(format, width, height, fsaa);
                mRenderBufferMap[key] = RBRef(rb);
                retval.buffer = rb;
                retval.zoffset = 0;
                retval.numSamples = fsaa;
            }
        }
//        std::cerr << "Requested renderbuffer with format " << std::hex << format << std::dec << " of " << width << "x" << height << " :" << retval.buffer << std::endl;
        return retval;
    }

    void GLES2FBOManager::requestRenderBuffer(const GLES2SurfaceDesc &surface)
    {
        if(surface.buffer == 0)
            return;
        RBFormat key(surface.buffer->getGLFormat(), surface.buffer->getWidth(), surface.buffer->getHeight(), surface.numSamples);
        RenderBufferMap::iterator it = mRenderBufferMap.find(key);
        assert(it != mRenderBufferMap.end());
        if (it != mRenderBufferMap.end())   // Just in case
        {
            assert(it->second.buffer == surface.buffer);
            // Increase refcount
            ++it->second.refcount;
        }
    }

    void GLES2FBOManager::releaseRenderBuffer(const GLES2SurfaceDesc &surface)
    {
        if(surface.buffer == 0)
            return;
        RBFormat key(surface.buffer->getGLFormat(), surface.buffer->getWidth(), surface.buffer->getHeight(), surface.numSamples);
        RenderBufferMap::iterator it = mRenderBufferMap.find(key);
        if(it != mRenderBufferMap.end())
        {
            // Decrease refcount
            --it->second.refcount;
            if(it->second.refcount==0)
            {
                // If refcount reaches zero, delete buffer and remove from map
                OGRE_DELETE it->second.buffer;
                mRenderBufferMap.erase(it);
                //std::cerr << "Destroyed renderbuffer of format " << std::hex << key.format << std::dec
                //        << " of " << key.width << "x" << key.height << std::endl;
            }
        }
    }

    GLuint GLES2FBOManager::getTemporaryFBO(size_t i)
    {
        assert(i < mTempFBO.size());

        return mTempFBO[i];
    }
}
