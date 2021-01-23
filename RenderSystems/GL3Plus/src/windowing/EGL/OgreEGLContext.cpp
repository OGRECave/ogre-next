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

#include "OgreEGLContext.h"

#include "OgreEGLGLSupport.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreRoot.h"

namespace Ogre
{
    EGLContext::EGLContext( EGLDisplay eglDisplay, EGLGLSupport *glsupport, ::EGLConfig fbconfig,
                            ::EGLSurface drawable, ::EGLContext context ) :
        mConfig( fbconfig ),
        mGLSupport( glsupport ),
        mEglDisplay( eglDisplay ),
        mContext( 0 ),
        mExternalContext( false ),
        mDrawable( drawable )
    {
        GL3PlusRenderSystem *renderSystem =
            static_cast<GL3PlusRenderSystem *>( Root::getSingleton().getRenderSystem() );
        EGLContext *mainContext = static_cast<EGLContext *>( renderSystem->_getMainContext() );
        ::EGLContext shareContext = 0;

        if( mainContext )
        {
            shareContext = mainContext->mContext;
        }

        if( context )
        {
            mContext = context;
            mExternalContext = true;
        }
        else
        {
            mContext = mGLSupport->createNewContext( eglDisplay, mConfig, shareContext );
        }

        if( !mContext )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "Unable to create a suitable EGLContext",
                         "EGLContext::EGLContext" );
        }
    }
    //-------------------------------------------------------------------------
    EGLContext::~EGLContext()
    {
        endCurrent();

        if( !mExternalContext )
        {
            eglDestroyContext( mEglDisplay, mContext );
            EGL_CHECK_ERROR
        }

        mContext = NULL;

        GL3PlusRenderSystem *rs =
            static_cast<GL3PlusRenderSystem *>( Root::getSingleton().getRenderSystem() );

        rs->_unregisterContext( this );
    }
    //-------------------------------------------------------------------------
    void EGLContext::setCurrent() { eglMakeCurrent( mEglDisplay, mDrawable, mDrawable, mContext ); }
    //-------------------------------------------------------------------------
    void EGLContext::endCurrent() { eglMakeCurrent( mEglDisplay, 0, 0, 0 ); }
    //-------------------------------------------------------------------------
    GL3PlusContext *EGLContext::clone() const
    {
        return new EGLContext( mGLSupport, mGLSupport, mConfig, mDrawable );
    }
}  // namespace Ogre
