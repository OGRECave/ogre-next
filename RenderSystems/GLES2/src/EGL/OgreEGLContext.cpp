/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2008 Renato Araujo Oliveira Filho <renatox@gmail.com>
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
#include "OgreGLES2RenderSystem.h"

#include "OgreEGLSupport.h"
#include "OgreEGLContext.h"

#include "OgreRoot.h"

namespace Ogre {
    EGLContext::EGLContext(EGLDisplay eglDisplay,
                            const EGLSupport* glsupport,
                           ::EGLConfig glconfig,
                           ::EGLSurface drawable)
        : mGLSupport(glsupport),
          mContext(0)
    {
        assert(drawable);
        GLES2RenderSystem* renderSystem = static_cast<GLES2RenderSystem*>(Root::getSingleton().getRenderSystem());
        EGLContext* mainContext = static_cast<EGLContext*>(renderSystem->_getMainContext());
        ::EGLContext shareContext = (::EGLContext) 0;

        if (mainContext)
        {
            shareContext = mainContext->mContext;
        }

        _createInternalResources(eglDisplay, glconfig, drawable, shareContext);
    }

    EGLContext::~EGLContext()
    {
        GLES2RenderSystem *rs =
            static_cast<GLES2RenderSystem*>(Root::getSingleton().getRenderSystem());

        _destroyInternalResources();
        rs->_unregisterContext(this);
    }
    
    void EGLContext::_createInternalResources(EGLDisplay eglDisplay, ::EGLConfig glconfig, ::EGLSurface drawable, ::EGLContext shareContext)
    {
        mDrawable = drawable;
        mConfig = glconfig;
        mEglDisplay = eglDisplay;
        
        mContext = mGLSupport->createNewContext(mEglDisplay, mConfig, shareContext);
        
        if (!mContext)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Unable to create a suitable EGLContext",
                        "EGLContext::EGLContext");
        }

        setCurrent();

        // Initialise GL3W
        if (gleswInit())
            LogManager::getSingleton().logMessage("Failed to initialize GL3W");
    }
    
    void EGLContext::_destroyInternalResources()
    {
        endCurrent();
        
        eglDestroyContext(mEglDisplay, mContext);
        EGL_CHECK_ERROR
        
        mContext = NULL;
    }

    void EGLContext::setCurrent()
    {
        EGLBoolean ret = eglMakeCurrent(mEglDisplay,
                                        mDrawable, mDrawable, mContext);
        EGL_CHECK_ERROR
        if (!ret)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Fail to make context current",
                        __FUNCTION__);
        }
    }

    void EGLContext::endCurrent()
    {
        eglMakeCurrent(mEglDisplay, 0, 0, 0);
        EGL_CHECK_ERROR
    }

    EGLSurface EGLContext::getDrawable() const
    {
        return mDrawable;
    }
}
