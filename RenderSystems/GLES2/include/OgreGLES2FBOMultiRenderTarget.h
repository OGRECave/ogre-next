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

#ifndef __GLES2MULTIRENDERTARGET_H__
#define __GLES2MULTIRENDERTARGET_H__

#include "OgreGLES2FrameBufferObject.h"

namespace Ogre {
    
    class GLES2FBOManager;

    /** MultiRenderTarget for GL ES 2.x.
    */
    class _OgreGLES2Export GLES2FBOMultiRenderTarget : public MultiRenderTarget
    {
    public:
        GLES2FBOMultiRenderTarget(GLES2FBOManager *manager, const String &name);
        ~GLES2FBOMultiRenderTarget();

        virtual void getCustomAttribute( const String& name, void *pData );

        bool requiresTextureFlipping() const { return true; }

        /// Override so we can attach the depth buffer to the FBO
        virtual bool attachDepthBuffer( DepthBuffer *depthBuffer, bool exactFormatMatch );
        virtual void detachDepthBuffer();
        virtual void _detachDepthBuffer();
    private:
        virtual void bindSurfaceImpl(size_t attachment, RenderTexture *target);
        virtual void unbindSurfaceImpl(size_t attachment); 
        GLES2FrameBufferObject fbo;
    };

}

#endif // __GLES2MULTIRENDERTARGET_H__
