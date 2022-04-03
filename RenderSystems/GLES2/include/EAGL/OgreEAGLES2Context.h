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

#ifndef __EAGLES2Context_H__
#define __EAGLES2Context_H__

#include "OgreGLES2Context.h"

#ifdef __OBJC__
#   import <QuartzCore/CAEAGLLayer.h>
#endif

namespace Ogre {

    class _OgrePrivate EAGLES2Context : public GLES2Context
    {
        protected:
#ifdef __OBJC__
            CAEAGLLayer *mDrawable;
            EAGLContext *mContext;
#else
            void *mDrawablePlaceholder;
            void *mContextPlaceholder;
#endif

        public:
#ifdef __OBJC__
            EAGLES2Context(CAEAGLLayer *drawable, EAGLSharegroup *group);
            CAEAGLLayer * getDrawable() const;
            EAGLContext * getContext() const;
#endif
            virtual ~EAGLES2Context();

            virtual void setCurrent();
            virtual void endCurrent();
            virtual GLES2Context * clone() const;

            bool createFramebuffer();
            void destroyFramebuffer();

            /* The pixel dimensions of the backbuffer */
            GLint mBackingWidth;
            GLint mBackingHeight;

            /* OpenGL names for the renderbuffer and framebuffers used to render to this view */
            GLuint mViewRenderbuffer;
            GLuint mViewFramebuffer;

            /* OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist) */
            GLuint mDepthRenderbuffer;

            bool mIsMultiSampleSupported;
            GLsizei mNumSamples;
            GLuint mSampleFramebuffer;
            GLuint mSampleRenderbuffer;
    };
}

#endif
