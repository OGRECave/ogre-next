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

#ifndef _OgreEglPBufferWindow_H_
#define _OgreEglPBufferWindow_H_

#include "OgreWindow.h"

#include "PBuffer/OgreEglPBufferContext.h"
#include "PBuffer/OgreEglPBufferSupport.h"

namespace Ogre
{
    /**
    PBuffer implementation of EGL
        When supported, PBuffer is a surface-less implementation of OpenGL.
        This means the window is a dummy hidden window that can run without X11 or Wayland
        servers as long as DRM is supported.

        This is ideal for running a headless server in a terminal, an SSH session or the cloud

        While support has been geared mostly towards Linux, it may run on Windows.
        However I do not know whether drivers support it.

        Pass parameter "Device" to RenderSystem::setConfigOption.
        Mesa drivers typically expose "EGL_EXT_device_drm" (an actual GPU) and
        "EGL_MESA_device_software" (SW emulation)

    @remarks
        Due to drivers not caring too much about PBuffer support
        (e.g. sRGB is an afterthought, depth buffers may not be 32-bits)
        we create a 1x1 PBuffer to get OpenGL up, but we don't expose it to the user.

        Instead we create a regular FBO for rendering with the requested settings
        (resolution, sRGB, depth buffer). This means implementation details
        such as requiresTextureFlipping must behave the same way as regular FBOs,
        and is inconsistent with the other GL Window backends
    */
    class _OgrePrivate EglPBufferWindow : public Window
    {
    protected:
        bool mClosed;
        bool mVisible;
        bool mHidden;
        bool mHwGamma;

        EglPBufferSupport *mGLSupport;
        EglPBufferContext *mContext;

        EGLConfig mEglConfig;
        EGLSurface mEglSurface;

        void create( const NameValuePairList *miscParams );

    public:
        EglPBufferWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                          const NameValuePairList *miscParams, Ogre::EglPBufferSupport *glsupport );
        virtual ~EglPBufferWindow();

        virtual void _initialize( TextureGpuManager *textureManager );

        virtual void setVSync( bool vSync, uint32 vSyncInterval );
        virtual void reposition( int32 left, int32 top );

        void requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                      uint32 width, uint32 height, uint32 frequencyNumerator,
                                      uint32 frequencyDenominator );

        /** @copydoc see RenderWindow::destroy */
        virtual void destroy( void );

        /** @copydoc see RenderWindow::isClosed */
        virtual bool isClosed( void ) const;

        /** @copydoc see RenderWindow::isVisible */
        bool isVisible( void ) const;

        virtual void _setVisible( bool visible );

        /** @copydoc see RenderWindow::isHidden */
        bool isHidden( void ) const { return mHidden; }

        /** @copydoc see RenderWindow::setHidden */
        void setHidden( bool hidden );

        /** @copydoc see RenderWindow::resize */
        void requestResolution( uint32 width, uint32 height );

        /** @copydoc see RenderWindow::swapBuffers */
        void swapBuffers();

        /**
           @remarks
           * Get custom attribute; the following attributes are valid:
           * GLCONTEXT    The Ogre GL3PlusContext used for rendering.
           */
        virtual void getCustomAttribute( IdString name, void *pData );

        bool requiresTextureFlipping() const { return true; }
    };
}  // namespace Ogre

#endif
