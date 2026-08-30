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

#include "windowing/EGL/Wayland/OgreWaylandEglContext.h"

#include "OgreException.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreRoot.h"

namespace Ogre
{
    //-------------------------------------------------------------------------
    WaylandEglContext::WaylandEglContext( WaylandEglSupport *support, EGLSurface eglSurface ) :
        mGLSupport( support ),
        mEglDisplay( support->getEglDisplay() ),
        mEglSurface( eglSurface ),
        mEglContext( EGL_NO_CONTEXT )
    {
        mEglContext = eglCreateContext( mEglDisplay, mGLSupport->getEglConfig(),
                                         mGLSupport->getSharedContext(), 0 );

        if( mEglContext == EGL_NO_CONTEXT )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "Unable to create a WaylandEglContext",
                         "WaylandEglContext::WaylandEglContext" );
        }
    }
    //-------------------------------------------------------------------------
    WaylandEglContext::~WaylandEglContext()
    {
        endCurrent();

        GL3PlusRenderSystem *rs =
            static_cast<GL3PlusRenderSystem *>( Root::getSingleton().getRenderSystem() );
        rs->_unregisterContext( this );

        if( mEglContext != EGL_NO_CONTEXT )
            eglDestroyContext( mEglDisplay, mEglContext );
    }
    //-------------------------------------------------------------------------
    void WaylandEglContext::setCurrent()
    {
        eglMakeCurrent( mEglDisplay, mEglSurface, mEglSurface, mEglContext );
    }
    //-------------------------------------------------------------------------
    void WaylandEglContext::endCurrent()
    {
        eglMakeCurrent( mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    }
    //-------------------------------------------------------------------------
    GL3PlusContext *WaylandEglContext::clone() const
    {
        return new WaylandEglContext( mGLSupport, mEglSurface );
    }
}  // namespace Ogre
