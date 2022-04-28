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
#ifndef __OgreGLES2FBORTT_H__
#define __OgreGLES2FBORTT_H__

#include "OgreGLES2RenderTexture.h"
#include "OgreGLES2Context.h"
#include "OgreGLES2FrameBufferObject.h"
#include "OgreGLES2ManagedResource.h"

namespace Ogre {
    class GLES2FBOManager;

    /** RenderTexture for GL ES 2 FBO
    */
    class _OgreGLES2Export GLES2FBORenderTexture: public GLES2RenderTexture MANAGED_RESOURCE
    {
    public:
        GLES2FBORenderTexture(GLES2FBOManager *manager, const String &name, const GLES2SurfaceDesc &target, bool writeGamma, uint fsaa);
        
        virtual void getCustomAttribute(const String& name, void* pData);

        /// Override needed to deal with multisample buffers
        virtual void swapBuffers();

        /// Override so we can attach the depth buffer to the FBO
        virtual bool attachDepthBuffer( DepthBuffer *depthBuffer, bool exactFormatMatch );
        virtual void detachDepthBuffer();
        virtual void _detachDepthBuffer();
    protected:
        GLES2FrameBufferObject mFB;
        
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
        /** See AndroidResource. */
        virtual void notifyOnContextLost();
        
        /** See AndroidResource. */
        virtual void notifyOnContextReset();
#endif
    };
    
    /** Factory for GL ES 2 Frame Buffer Objects, and related things.
    */
    class _OgreGLES2Export GLES2FBOManager: public GLES2RTTManager 
    {
    public:
        GLES2FBOManager();
        ~GLES2FBOManager();
        
        /** Bind a certain render target if it is a FBO. If it is not a FBO, bind the
            main frame buffer.
        */
        void bind(RenderTarget *target);
        
        /** Unbind a certain render target. No-op for FBOs.
        */
        void unbind(RenderTarget *target) {};
        
        /** Get best depth and stencil supported for given internalFormat
        */
        virtual void getBestDepthStencil(PixelFormat depthFormat, PixelFormat fboFormat,
                                         GLenum *outDepthFormat, GLenum *outStencilFormat);
        
        /** Create a texture rendertarget object
        */
        virtual GLES2FBORenderTexture *createRenderTexture(const String &name, 
            const GLES2SurfaceDesc &target, bool writeGamma, uint fsaa);

        /** Create a multi render target 
        */
        virtual MultiRenderTarget* createMultiRenderTarget(const String & name);
        
        /** Request a render buffer. If format is GL_NONE, return a zero buffer.
        */
        GLES2SurfaceDesc requestRenderBuffer(GLenum format, uint32 width, uint32 height, uint fsaa);
        /** Request the specify render buffer in case shared somewhere. Ignore
            silently if surface.buffer is 0.
        */
        void requestRenderBuffer(const GLES2SurfaceDesc &surface);
        /** Release a render buffer. Ignore silently if surface.buffer is 0.
        */
        void releaseRenderBuffer(const GLES2SurfaceDesc &surface);
        
        /** Check if a certain format is usable as FBO rendertarget format
        */
        bool checkFormat(PixelFormat format) { return mProps[format].valid; }
        
        /** Get a FBO without depth/stencil for temporary use, like blitting between textures.
        */
        GLuint getTemporaryFBO(size_t i);
        
        /** Detects all supported fbo's and recreates the temporary fbo */
        void _reload();
        
    private:
        /** Frame Buffer Object properties for a certain texture format.
        */
        struct FormatProperties
        {
            bool valid; // This format can be used as RTT (FBO)
            
            /** Allowed modes/properties for this pixel format
            */
            struct Mode
            {
                size_t depth;     // Depth format (0=no depth)
                size_t stencil;   // Stencil format (0=no stencil)
            };
            
            vector<Mode>::type modes;
        };
        /** Properties for all internal formats defined by OGRE
        */
        FormatProperties mProps[PF_COUNT];
        
        /** Stencil and depth renderbuffers of the same format are re-used between surfaces of the 
            same size and format. This can save a lot of memory when a large amount of rendertargets
            are used.
        */
        struct RBFormat
        {
            RBFormat(GLenum inFormat, size_t inWidth, size_t inHeight, uint fsaa):
                format(inFormat), width(inWidth), height(inHeight), samples(fsaa)
            {}
            RBFormat() {}
            GLenum format;
            size_t width;
            size_t height;
            uint samples;
            // Overloaded comparison operator for usage in map
            bool operator < (const RBFormat &other) const
            {
                if(format < other.format)
                {
                    return true;
                }
                else if(format == other.format)
                {
                    if(width < other.width)
                    {
                        return true;
                    }
                    else if(width == other.width)
                    {
                        if(height < other.height)
                            return true;
                        else if (height == other.height)
                        {
                            if (samples < other.samples)
                                return true;
                        }
                    }
                }
                return false;
            }
        };
        struct RBRef
        {
            RBRef(){}
            RBRef(v1::GLES2RenderBuffer *inBuffer):
                buffer(inBuffer), refcount(1)
            { }
            v1::GLES2RenderBuffer *buffer;
            size_t refcount;
        };
        typedef map<RBFormat, RBRef>::type RenderBufferMap;
        RenderBufferMap mRenderBufferMap;
        
        /** Temporary FBO identifier
         */
        std::vector<GLuint> mTempFBO;
        
        /** Detect allowed FBO formats */
        void detectFBOFormats();
        GLuint _tryFormat(GLenum depthFormat, GLenum stencilFormat);
        bool _tryPackedFormat(GLenum packedFormat);
        void _createTempFramebuffer(PixelFormat pixFmt, GLuint internalFormat, GLuint fmt, GLenum dataType, GLuint &fb, GLuint &tid);
    };
}

#endif
