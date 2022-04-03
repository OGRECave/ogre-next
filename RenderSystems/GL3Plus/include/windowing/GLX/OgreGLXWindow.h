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

#ifndef _OgreGLXWindow_H_
#define _OgreGLXWindow_H_
#include "OgreWindow.h"

#include <X11/Xlib.h>
#include "OgreGLXContext.h"
#include "OgreGLXGLSupport.h"

namespace Ogre
{
    class _OgrePrivate GLXWindow final : public Window
    {
    protected:
        bool mClosed;
        bool mVisible;
        bool mHidden;
        bool mIsTopLevel;
        bool mIsExternal;
        bool mIsExternalGLControl;

        GLXGLSupport *mGLSupport;
        ::Window      mWindow;
        GLXContext   *mContext;

        void switchFullScreen( bool fullscreen );

        void create( PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams );

    public:
        GLXWindow( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                   PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams,
                   GLXGLSupport *glsupport );
        ~GLXWindow() override;

        void _initialize( TextureGpuManager *textureManager ) override;

        void setVSync( bool vSync, uint32 vSyncInterval ) override;
        void reposition( int32 left, int32 top ) override;

        void requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                      uint32 width, uint32 height, uint32 frequencyNumerator,
                                      uint32 frequencyDenominator ) override;

        /** @copydoc see RenderWindow::destroy */
        virtual void destroy() override;

        /** @copydoc see RenderWindow::isClosed */
        virtual bool isClosed() const override;

        /** @copydoc see RenderWindow::isVisible */
        bool isVisible() const override;

        virtual void _setVisible( bool visible ) override;

        /** @copydoc see RenderWindow::isHidden */
        bool isHidden() const override { return mHidden; }

        /** @copydoc see RenderWindow::setHidden */
        void setHidden( bool hidden ) override;

        /** @copydoc see RenderWindow::resize */
        void requestResolution( uint32 width, uint32 height ) override;

        /** @copydoc see RenderWindow::windowMovedOrResized */
        void windowMovedOrResized() override;

        /** @copydoc see RenderWindow::swapBuffers */
        void swapBuffers() override;

        /**
           @remarks
           * Get custom attribute; the following attributes are valid:
           * WINDOW      The X Window target for rendering.
           * GLCONTEXT    The Ogre GL3PlusContext used for rendering.
           * DISPLAY        The X Display connection behind that context.
           * DISPLAYNAME    The X Server name for the connected display.
           * ATOM          The X Atom used in client delete events.
           */
        virtual void getCustomAttribute( IdString name, void *pData ) override;

        bool requiresTextureFlipping() const { return false; }
    };
}  // namespace Ogre

#endif
