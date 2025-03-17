/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-present Torus Knot Software Ltd

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

#pragma once

#include "OgreVulkanWindow.h"

#include <windows.h>

namespace Ogre
{
    class VulkanWin32Window final : public VulkanWindowSwapChainBased
    {
    private:
        HINSTANCE mHinstance;
        HWND mHwnd;  // Win32 Window handle
        HDC mHDC;
        uint32 mColourDepth;
        bool mIsExternal;
        char *mDeviceName;
        bool mSizing;
        bool mHidden;
        bool mVisible;
        DWORD mWindowedWinStyle;    // Windowed mode window style flags.
        DWORD mFullscreenWinStyle;  // Fullscreen mode window style flags.

        static bool mClassRegistered;

        bool updateWindowRect();
        void adjustWindow( uint32 clientWidth, uint32 clientHeight,  //
                           uint32 *outDrawableWidth, uint32 *outDrawableHeight );

        /// Return the target window style depending on the fullscreen parameter.
        DWORD getWindowStyle( bool fullScreen ) const;

        void createWindow( const NameValuePairList *miscParams );
        void createSurface() override;

    public:
        VulkanWin32Window( const String &title, uint32 width, uint32 height, bool fullscreenMode );
        ~VulkanWin32Window() override;

        static const char *getRequiredExtensionName();

        void reposition( int32 left, int32 top ) override;
        void requestResolution( uint32 width, uint32 height ) override;
        void requestFullscreenSwitch( bool goFullscreen, bool borderless, uint32 monitorIdx,
                                      uint32 width, uint32 height, uint32 frequencyNumerator,
                                      uint32 frequencyDenominator ) override;
        void windowMovedOrResized() override;

        void _setVisible( bool visible ) override;
        bool isVisible() const override;
        void setHidden( bool hidden ) override;
        bool isHidden() const override;
        void setFocused( bool focused ) override;
        void _initialize( TextureGpuManager *textureGpuManager,
                          const NameValuePairList *miscParams ) override;

        void destroy() override;

        void getCustomAttribute( IdString name, void *pData );
    };

}  // namespace Ogre
