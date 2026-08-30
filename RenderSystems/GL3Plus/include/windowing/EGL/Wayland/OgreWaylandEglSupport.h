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
#ifndef OGRE_WaylandEglSupport_H
#define OGRE_WaylandEglSupport_H

#include "OgreGL3PlusSupport.h"

// Must be defined before the EGL headers are first included anywhere in a
// translation unit that touches this file: it makes <EGL/eglplatform.h>
// define EGLNativeWindowType/EGLNativeDisplayType as wl_egl_window*/
// wl_display* instead of defaulting to the X11 typedefs.
#ifndef WL_EGL_PLATFORM
#    define WL_EGL_PLATFORM
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <wayland-client.h>

#ifdef None
#    undef None
#endif

namespace Ogre
{
    /** GL3PlusSupport implementation that creates OpenGL contexts against a
        host-owned Wayland display using EGL_PLATFORM_WAYLAND_KHR.

        @remarks
            This backend only supports embedding into a Wayland surface that the
            caller (e.g. Qt/QML, as used by Gazebo) already owns and is driving
            (see WaylandEglWindow). It does not connect its own wl_display, does
            not bind any Wayland globals, and does not dispatch Wayland protocol
            events. A standalone, self-owned toplevel window mode is not
            implemented (deferred, see WaylandEglWindow::create).
    */
    class _OgrePrivate WaylandEglSupport : public GL3PlusSupport
    {
    public:
        /// Which entry points were used to obtain mEglDisplay, and therefore
        /// which entry points WaylandEglWindow must use to create a matching
        /// EGLSurface: mixing e.g. a core EGL_PLATFORM_WAYLAND_KHR display
        /// with the EGLNativeWindowType-based eglCreateWindowSurface (or vice
        /// versa) is undefined behaviour per the EGL spec, and mixing the
        /// core eglCreatePlatformWindowSurface with an EXT-obtained display
        /// risks calling into a statically-linked core symbol a driver that
        /// only implements the EXT extension may not even export.
        enum PlatformMode
        {
            /// eglGetDisplay() + eglCreateWindowSurface() with
            /// EGLNativeDisplayType/EGLNativeWindowType.
            PM_LEGACY,
            /// eglGetPlatformDisplay() + eglCreatePlatformWindowSurface()
            /// (EGL 1.5 core).
            PM_CORE_1_5,
            /// eglGetPlatformDisplayEXT() + eglCreatePlatformWindowSurfaceEXT()
            /// (EGL 1.4 + EGL_EXT_platform_wayland).
            PM_EXT
        };

    protected:
        wl_display *mWlDisplay;
        EGLDisplay  mEglDisplay;
        EGLConfig   mEglConfig;
        ::EGLContext mSharedContext;

        PlatformMode mPlatformMode;

        /// Picks mEglConfig from mEglDisplay, walking a fallback ladder of
        /// attribute sets from most to least preferred.
        void chooseEglConfig();

        /// Creates the shared root EGLContext (mSharedContext) that every
        /// window's own context is created to share GL object namespace with.
        /// Walks GL core-profile versions from 4.5 down to 3.3.
        ::EGLContext createSharedContext();

        void terminate();

    public:
        WaylandEglSupport();
        ~WaylandEglSupport() override;

        /// Connects EGL to waylandDisplay (via eglGetPlatformDisplay) and
        /// creates the shared context, unless already initialised against the
        /// same wl_display. If already initialised against a *different*
        /// wl_display, the old EGLDisplay/context is torn down first.
        void initialise( wl_display *waylandDisplay );

        EGLDisplay   getEglDisplay() const { return mEglDisplay; }
        EGLConfig    getEglConfig() const { return mEglConfig; }
        ::EGLContext getSharedContext() const { return mSharedContext; }
        PlatformMode getPlatformMode() const { return mPlatformMode; }

        /// @copydoc GL3PlusSupport::addConfig
        void addConfig() override;

        /// @copydoc GL3PlusSupport::validateConfig
        String validateConfig() override;

        /// @copydoc GL3PlusSupport::createWindow
        Window *createWindow( bool autoCreateWindow, GL3PlusRenderSystem *renderSystem,
                               const String &windowTitle ) override;

        /// @copydoc Root::createRenderWindow
        Window *newWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                            const NameValuePairList *miscParams = 0 ) override;

        /// @copydoc GL3PlusSupport::start
        void start() override;

        /// @copydoc GL3PlusSupport::stop
        void stop() override;

        /// @copydoc GL3PlusSupport::getProcAddress
        void *getProcAddress( const char *procname ) const override;
    };
}  // namespace Ogre

#endif  // OGRE_WaylandEglSupport_H
