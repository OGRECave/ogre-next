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
#ifndef __OgreEGLContext_H__
#define __OgreEGLContext_H__

#include "OgreGL3PlusContext.h"

#include "OgreEGLGLSupport.h"

namespace Ogre
{
    class _OgrePrivate EGLContext : public GL3PlusContext
    {
        EGLConfig     mConfig;
        EGLGLSupport *mGLSupport;
        EGLDisplay    mEglDisplay;
        ::EGLContext  mContext;
        bool          mExternalContext;

    public:
        EGLSurface mDrawable;

        EGLContext( EGLDisplay eglDisplay, EGLGLSupport *glsupport, ::EGLConfig fbconfig,
                    ::EGLSurface drawable, ::EGLContext context = 0 );

        virtual ~EGLContext();

        /// @copydoc GL3PlusContext::setCurrent
        virtual void setCurrent();

        /// @copydoc GL3PlusContext::endCurrent
        virtual void endCurrent();

        /// @copydoc GL3PlusContext::clone
        GL3PlusContext *clone() const;
    };
}  // namespace Ogre

#endif
