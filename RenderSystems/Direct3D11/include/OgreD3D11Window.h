/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#ifndef _OgreD3D11Window_H_
#define _OgreD3D11Window_H_

#include "OgreD3D11Prerequisites.h"
#include "OgreWindow.h"
#include "OgreCommon.h"

namespace Ogre
{
    class _OgreD3D11Export D3D11Window : public Window
    {
    protected:
        D3D11Device     &mDevice;
        /// Whether window was not created by Ogre
        bool mIsExternal;
        bool mSizing;
        bool mClosed;
        bool mHidden;
        bool mAlwaysWindowedMode;
        bool mHwGamma;
        bool mVisible;

        // Window size depended resources - must be released
        // before swapchain resize and recreated later
        ComPtr<ID3D11Texture2D> mpBackBuffer;
        /// Optional, always holds up-to-date copy data from mpBackBuffer if not NULL
        ComPtr<ID3D11Texture2D> mpBackBufferInterim;

        D3D11RenderSystem       *mRenderSystem;

    protected:
        virtual PixelFormatGpu _getRenderFormat() { return mHwGamma ? PFG_BGRA8_UNORM_SRGB : PFG_BGRA8_UNORM; } // preferred since Win8

    public:
        D3D11Window( const String &title, uint32 width, uint32 height,
                     bool fullscreenMode, PixelFormatGpu depthStencilFormat,
                     const NameValuePairList *miscParams,
                     D3D11Device &device, D3D11RenderSystem *renderSystem );
        virtual ~D3D11Window();
        virtual void destroy();

        virtual void reposition(int leftPt, int topPt)          {}

        virtual bool isClosed() const                           { return mClosed; }
        virtual void _setVisible( bool visible )                { mVisible = visible; }
        virtual void setHidden( bool hidden )                   { mHidden = hidden; }
        virtual bool isHidden() const                           { return mHidden; }

        virtual void getCustomAttribute( IdString name, void* pData );
    };

    class _OgreD3D11Export D3D11WindowSwapChainBased : public D3D11Window
    {
    protected:
        ComPtr<IDXGISwapChain>  mSwapChain;
        ComPtr<IDXGISwapChain1> mSwapChain1;
        //DXGI_SWAP_CHAIN_DESC_N  mSwapChainDesc;

        /// Flag to determine if the swapchain flip model is active.
        /// Not supported before Win8.0, required for WinRT.
        bool mUseFlipMode;

        // We save the previous present stats - so we can detect a "vblank miss"
        DXGI_FRAME_STATISTICS   mPreviousPresentStats;
        // Does mLastPresentStats data is valid (it isn't if when you start or resize the window)
        bool                    mPreviousPresentStatsIsValid;
        // Number of times we missed the v sync blank
        uint32                  mVBlankMissCount;

    protected:
        DXGI_FORMAT _getSwapChainFormat();
        uint8 _getSwapChainBufferCount(void) const;
        void _createSwapChain();
        virtual HRESULT _createSwapChainImpl() = 0;
        void _destroySwapChain();
        void _createSizeDependedD3DResources();
        void _destroySizeDependedD3DResources();
        void resizeSwapChainBuffers( uint32 width, uint32 height );
        void notifyResolutionChanged(void);

    public:
        D3D11WindowSwapChainBased( const String &title, uint32 width, uint32 height,
                                   bool fullscreenMode, PixelFormatGpu depthStencilFormat,
                                   const NameValuePairList *miscParams,
                                   D3D11Device &device, D3D11RenderSystem *renderSystem );
        virtual ~D3D11WindowSwapChainBased();

        virtual void _initialize( TextureGpuManager *textureGpuManager );
        virtual void destroy();

        /// @copydoc Window::setFsaa
        virtual void setFsaa(const String& fsaa);

        virtual void swapBuffers(void);
    };
}

#include "Windowing/WIN32/OgreD3D11WindowHwnd.h"
#include "Windowing/WIN32/OgreD3D11WindowWinRT.h"

#endif
