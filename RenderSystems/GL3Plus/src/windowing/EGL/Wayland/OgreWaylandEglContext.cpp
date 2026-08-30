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
    WaylandEglContext::WaylandEglContext( WaylandEglSupport *support, EGLSurface eglSurface,
                                           ::EGLContext externalContext ) :
        mGLSupport( support ),
        mEglDisplay( support->getEglDisplay() ),
        mEglSurface( eglSurface ),
        mEglContext( EGL_NO_CONTEXT ),
        mExternalContext( false )
    {
        if( externalContext != EGL_NO_CONTEXT )
        {
            mEglContext = externalContext;
            mExternalContext = true;
        }
        else
        {
            // Mirrors GLXContext: look up the RenderSystem's current main
            // context fresh (not cached) and share GL object namespace with
            // it. If there is no main context yet, this is the very first
            // context created - it shares with nothing and will itself
            // become the main context once GL3PlusRenderSystem registers it
            // (via the owning window's "GLCONTEXT" custom attribute).
            GL3PlusRenderSystem *renderSystem =
                static_cast<GL3PlusRenderSystem *>( Root::getSingleton().getRenderSystem() );
            GL3PlusContext *mainContext = renderSystem->_getMainContext();
            ::EGLContext    shareContext = EGL_NO_CONTEXT;

            if( mainContext )
                shareContext = static_cast<WaylandEglContext *>( mainContext )->getEglContext();

            mEglContext = mGLSupport->createContext( shareContext );
        }

        if( mEglContext == EGL_NO_CONTEXT )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "Unable to create a WaylandEglContext",
                         "WaylandEglContext::WaylandEglContext" );
        }
    }
    //-------------------------------------------------------------------------
    WaylandEglContext::~WaylandEglContext()
    {
        // Deliberately does NOT call endCurrent() here - mirrors GLXContext's
        // destructor exactly (it never un-currents on teardown either).
        // eglDestroyContext() is safe to call on a still-current context
        // (EGL only actually frees it once it stops being current); calling
        // eglMakeCurrent(..., EGL_NO_CONTEXT) here would be wrong for an
        // externally-adopted context in particular - Ogre doesn't own it
        // and has no business un-currenting it out from under the caller
        // (e.g. Qt), which is exactly what happened here: destroying a
        // failed window mid-retry (see GL3PlusRenderSystem::_createRenderWindow's
        // cleanup-on-exception path) used to clear the *caller's* adopted
        // context, making every subsequent retry see EGL_NO_CONTEXT.
        GL3PlusRenderSystem *rs =
            static_cast<GL3PlusRenderSystem *>( Root::getSingleton().getRenderSystem() );
        rs->_unregisterContext( this );

        if( mEglContext != EGL_NO_CONTEXT && !mExternalContext )
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
