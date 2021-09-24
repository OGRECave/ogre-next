/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#ifndef __OSXCocoaWindow_H__
#define __OSXCocoaWindow_H__

#include "OgreOSXCocoaContext.h"

#include <AppKit/NSWindow.h>
#include <QuartzCore/CVDisplayLink.h>
#include "OgreOSXCocoaView.h"
#include "OgreOSXCocoaWindowDelegate.h"

#include "OgreWindow.h"
#include "OgreTextureGpu.h"

@class CocoaWindowDelegate;

@interface OgreGL3PlusWindow : NSWindow

@end

namespace Ogre
{
    class _OgreGL3PlusExport CocoaWindow : public Window
    {
    private:
        NSWindow *mWindow;
        NSView *mView;
        NSOpenGLContext *mGLContext;
        NSOpenGLPixelFormat *mGLPixelFormat;
        // CVDisplayLinkRef mDisplayLink;
        NSPoint mWindowOriginPt;
        CocoaWindowDelegate *mWindowDelegate;
        CocoaContext* mContext;

        bool mClosed;
        bool mVisible;
        bool mHidden;
        bool mIsExternal;

        // Legacy
        bool mActive;
        bool mAutoDeactivatedOnFocusChange;

        bool mHwGamma;
        bool mVSync;
        bool mHasResized;
        String mWindowTitle;
        bool mUseOgreGLView;
        float mContentScalingFactor;
        
        int _getPixelFromPoint(int viewPt) const;
        void _setWindowParameters(unsigned int widthPt, unsigned int heightPt);

    public:
        CocoaWindow(const String &title, uint32 widthPt, uint32 heightPt, bool fullscreenMode);
        virtual ~CocoaWindow();

        /** @copydoc see Window::_initialize */
        void _initialize(TextureGpuManager *textureManager) override;

        /** @copydoc see Window::setVSync */
        // void setVSync(bool vSync, uint32 vSyncInterval) override;

        /** @copydoc see Window::getViewPointToPixelScale */
        float getViewPointToPixelScale() const override;

        /** @copydoc see Window::destroy */
        void destroy(void) override;

        /** @copydoc see Window::isVisible */
        bool isVisible( void ) const override;

        /** @copydoc see Window::_setVisible */
        void _setVisible( bool visible ) override;

        /** @copydoc see Window::isClosed */
        bool isClosed(void) const override;

        /** @copydoc see Window::isHidden */
        bool isHidden(void) const override { return mHidden; }

        /** @copydoc see RenderWindow::setHidden */
        void setHidden(bool hidden) override;

        /** @copydoc see Window::reposition */
        void reposition(int leftPt, int topPt) override;

        /** @copydoc see Window::swapBuffers */
        void swapBuffers() override;

        /** @copydoc see Window::windowMovedOrResized */
        void windowMovedOrResized(void) override;

        /** @copydoc see Window::getCustomAttribute */
        /**
           @remarks
           * Get custom attribute; the following attributes are valid:
           * GLCONTEXT
           * WINDOW
           * VIEW
           * NSOPENGLCONTEXT
           * NSOPENGLPIXELFORMAT
           */
        void getCustomAttribute(IdString name, void* pData);

    public:
        // Required by CocoaWindowDelegate
        void setVisible(bool visible);

        // Legacy: from RenderTarget, RenderWindow 
        bool isActive() const;
        void setActive(bool value);
        bool isDeactivatedOnFocusChange() const;
        void setDeactivateOnFocusChange(bool deactivate);

        void create(const String &name, unsigned int widthPt, unsigned int heightPt,
            bool fullScreen, const NameValuePairList *miscParams);

    private:
        void _createNewWindow(String title, unsigned int widthPt, unsigned int heightPt);

    };
}

#endif // __OSXCocoaWindow_H__
