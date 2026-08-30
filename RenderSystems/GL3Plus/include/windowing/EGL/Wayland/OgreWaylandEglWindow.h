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
#ifndef _OgreWaylandEglWindow_H_
#define _OgreWaylandEglWindow_H_

#include "OgreWindow.h"

#include "windowing/EGL/Wayland/OgreWaylandEglContext.h"
#include "windowing/EGL/Wayland/OgreWaylandEglSupport.h"

#include <wayland-egl.h>

namespace Ogre
{
    /** Native Wayland window, embedding into a caller-owned wl_surface via EGL.

        @remarks
            This window ONLY supports embedding into an existing, host-owned
            Wayland connection and surface (e.g. a QQuickWindow's wl_surface,
            as used by Gazebo). Both "externalWaylandDisplay" (a wl_display*)
            and "externalWaylandSurface" (a wl_surface*) miscParams MUST be
            supplied to create(), stringified as the raw pointer value (see
            StringConverter::parseSizeT). There is currently no self-owned
            standalone toplevel mode (own wl_display connection, own
            xdg_wm_base binding, own xdg_toplevel) - that is deferred future
            work, tracked separately, and NOT implemented here.

            This window never dispatches Wayland protocol events on the shared
            wl_display (no wl_display_dispatch*, no wl_display_roundtrip, no
            wl_surface_commit) and must never be registered with
            WindowEventUtilities::_addRenderWindow(). The host application
            (e.g. Qt) is solely responsible for driving the wl_display's event
            loop; calling eglSwapBuffers() on the wl_egl_window's EGLSurface is
            sufficient to attach+commit+damage the next buffer, per the EGL
            Wayland platform spec.

            @warning
            The host dispatching the wl_display is not optional bookkeeping,
            it is required for swapBuffers() to keep working. Verified against
            NVIDIA's proprietary EGL Wayland driver (and expected to hold for
            Mesa too, since it follows from the buffer-queue/release-event
            model rather than a driver quirk): the second (and every
            subsequent) eglSwapBuffers() call can block forever if nothing
            ever reads the wl_display's socket, because the driver's internal
            buffer queue is waiting on a wl_buffer::release event from the
            compositor that will never be delivered to a client that never
            dispatches. A host that pauses its own event loop (e.g. app
            suspended, window unmapped) will therefore stall this window's
            rendering thread inside swapBuffers() until dispatching resumes.
            Wayland platform spec.
    */
    class _OgrePrivate WaylandEglWindow : public Window
    {
    protected:
        bool mClosed;
        bool mVisible;
        bool mHidden;
        bool mHwGamma;

        WaylandEglSupport *mGLSupport;
        WaylandEglContext *mContext;

        wl_display *mWlDisplay;  // Not owned.
        wl_surface *mWlSurface;  // Not owned.

        wl_egl_window *mWlEglWindow;  // Owned.
        EGLSurface     mEglSurface;   // Owned.

        void create( const NameValuePairList *miscParams );

    public:
        WaylandEglWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                           const NameValuePairList *miscParams, WaylandEglSupport *glsupport );
        ~WaylandEglWindow() override;

        void _initialize( TextureGpuManager *textureManager,
                           const NameValuePairList *miscParams ) override;

        void setVSync( bool vSync, uint32 vSyncInterval ) override;
        void reposition( int32 left, int32 top ) override;

        void requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                       uint32 width, uint32 height, uint32 frequencyNumerator,
                                       uint32 frequencyDenominator ) override;

        /** @copydoc Window::destroy */
        void destroy() override;

        /** @copydoc Window::isClosed */
        bool isClosed() const override;

        bool isVisible() const override;

        void _setVisible( bool visible ) override;

        /** @copydoc Window::isHidden */
        bool isHidden() const override { return mHidden; }

        /** @copydoc Window::setHidden */
        void setHidden( bool hidden ) override;

        /** @copydoc Window::requestResolution */
        void requestResolution( uint32 width, uint32 height ) override;

        void windowMovedOrResized() override;

        /** @copydoc Window::swapBuffers */
        void swapBuffers() override;

        /**
           @remarks
           * Get custom attribute; the following attributes are valid:
           * GLCONTEXT            The Ogre GL3PlusContext used for rendering.
           * RENDERDOC_DEVICE     The EGLContext used for rendering.
           * RENDERDOC_WINDOW     The EGLSurface used for rendering.
           * EGLDISPLAY           The EGLDisplay used for rendering.
           * EGLCONTEXT           The EGLContext used for rendering.
           * EGLSURFACE           The EGLSurface used for rendering.
           * WAYLAND_DISPLAY      The host-owned wl_display.
           * WAYLAND_SURFACE      The host-owned wl_surface. Also used as a
           *                      sentinel by WindowEventUtilities::messagePump()
           *                      to skip this window (it never registers for
           *                      pumping); do not repurpose this key.
           * WAYLAND_EGL_WINDOW   The wl_egl_window owned by this window.
           */
        void getCustomAttribute( IdString name, void *pData ) override;

        /// This window renders directly to a real on-screen EGLSurface (like
        /// GLX), not to an intermediate FBO (like the EGL PBuffer backend),
        /// so no flip is required.
        bool requiresTextureFlipping() const { return false; }
    };
}  // namespace Ogre

#endif
