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

        /// Effective FSAA mode, limited by hardware capabilities
        DXGI_SAMPLE_DESC mMsaaDesc;

        // Window size depended resources - must be released
        // before swapchain resize and recreated later
        ID3D11Texture2D         *mpBackBuffer;
        /// Optional, always holds up-to-date copy data from mpBackBuffer if not NULL
        ID3D11Texture2D         *mpBackBufferNoMSAA;
        //ID3D11RenderTargetView  *mRenderTargetView;

        D3D11RenderSystem       *mRenderSystem;

        //IDXGIDeviceN* _queryDxgiDevice();       // release after use!

    public:
        D3D11Window( const String &title, uint32 width, uint32 height,
                     bool fullscreenMode, PixelFormatGpu depthStencilFormat,
                     const NameValuePairList *miscParams,
                     D3D11Device &device, D3D11RenderSystem *renderSystem );
        virtual ~D3D11Window();

        bool isClosed() const                                   { return mClosed; }
        bool isHidden() const                                   { return mHidden; }

        virtual void getCustomAttribute( IdString name, void* pData );
    };

    class _OgreD3D11Export D3D11WindowSwapChainBased : public D3D11Window
    {
    protected:
        IDXGISwapChain  *mSwapChain;
        IDXGISwapChain1 *mSwapChain1;
        //DXGI_SWAP_CHAIN_DESC_N  mSwapChainDesc;

        /// Flag to determine if the swapchain flip sequential model is enabled.
        /// Not supported before Win8.0, required for WinRT.
        bool mUseFlipSequentialMode;

        // We save the previous present stats - so we can detect a "vblank miss"
        DXGI_FRAME_STATISTICS   mPreviousPresentStats;
        // Does mLastPresentStats data is valid (it isn't if when you start or resize the window)
        bool                    mPreviousPresentStatsIsValid;
        // Number of times we missed the v sync blank
        uint32                  mVBlankMissCount;

    public:
        D3D11WindowSwapChainBased( const String &title, uint32 width, uint32 height,
                                   bool fullscreenMode, PixelFormatGpu depthStencilFormat,
                                   const NameValuePairList *miscParams,
                                   D3D11Device &device, D3D11RenderSystem *renderSystem );
        virtual ~D3D11WindowSwapChainBased();
    };
}

#include "Windowing/WIN32/OgreD3D11WindowHwnd.h"

#endif
