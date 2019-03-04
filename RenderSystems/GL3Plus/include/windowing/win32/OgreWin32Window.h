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

#ifndef __Win32Window_H__
#define __Win32Window_H__

#include "OgreWin32Prerequisites.h"
#include "OgreWindow.h"

namespace Ogre {
    class _OgreGL3PlusExport Win32Window : public Window
    {
    protected:
        Win32GLSupport &mGLSupport;
        HWND    mHwnd;                  // Win32 Window handle
        HDC     mHDC;
        HGLRC   mGlrc;
        uint32  mColourDepth;
        bool    mIsExternal;
        char*   mDeviceName;
        bool    mIsExternalGLControl;
        bool    mOwnsGLContext;
        bool    mSizing;
        bool    mClosed;
        bool    mHidden;
        bool    mVisible;
        bool    mHwGamma;
        uint8   mMsaaCount;
        Win32Context *mContext;
        DWORD   mWindowedWinStyle;      // Windowed mode window style flags.
        DWORD   mFullscreenWinStyle;    // Fullscreen mode window style flags.

        static bool mClassRegistered;

        void create( PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams );

        void updateWindowRect(void);
        void adjustWindow( uint32 clientWidth, uint32 clientHeight,
                           uint32 *outDrawableWidth, uint32 *outDrawableHeight );
        /// Return the target window style depending on the fullscreen parameter.
        DWORD getWindowStyle( bool fullScreen ) const;

        void notifyResolutionChanged(void);

    public:
        Win32Window( const String &title, uint32 width, uint32 height, bool fullscreenMode,
                     PixelFormatGpu depthStencilFormat, const NameValuePairList *miscParams,
                     Win32GLSupport &glsupport );
        virtual ~Win32Window();

        virtual void _initialize( TextureGpuManager *textureGpuManager );
        virtual void destroy(void);

        virtual void reposition( int32 left, int32 top );
        virtual void requestResolution( uint32 width, uint32 height );
        virtual void requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                              uint32 width, uint32 height,
                                              uint32 frequencyNumerator, uint32 frequencyDenominator );

        virtual void windowMovedOrResized(void);

        bool isClosed(void) const;
        virtual void _setVisible( bool visible );
        virtual bool isVisible(void) const;
        virtual void setHidden( bool hidden );
        virtual bool isHidden(void) const;
        virtual void setVSync( bool vSync, uint32 vSyncInterval );
        virtual void swapBuffers(void);
        virtual void setFocused( bool focused );

        HWND getWindowHandle() const        { return mHwnd; }
        HDC getHDC() const                  { return mHDC; }

        void getCustomAttribute( IdString name, void* pData );
    };
}

#endif
